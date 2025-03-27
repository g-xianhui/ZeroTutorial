#include "player.h"
#include "space.h"
#include "wheel_timer.h"
#include "math_utils.h"
#include "network/tcp_connection.h"
#include "proto/space_service.pb.h"

void Player::send_msg(const char* msg_bytes, size_t n)
{
    if (_conn)
        _conn->send_msg(msg_bytes, n);
}

void Player::enter_space(Space* space)
{
    assert(_space == nullptr);
    _space = space;
}

void Player::leave_space()
{
    assert(_space != nullptr);
    _space = nullptr;
}

constexpr const char* combo_animations[] = { "Attack01", "Attack02" };

void Player::normal_attack(int combo_seq)
{
    // TODO 攻速限定检测
    if (combo_seq < 0 || combo_seq >= sizeof(combo_animations) / sizeof(const char*))
        return;

    // 这里直接告知客户端播放某个动画，而不是告知客户端执行normal_attack，原因是普攻（包括后面的技能）可能做得很复杂，有多种效果，随机或玩家选择，
    // 因此直接让服务器来算这个逻辑，告知客户端结果算了。如果动画很长，可以把animation的基础信息记录下来，新玩家上线时同步过去就是了。
    // 另一方面，技能动画可能是带root motion的，如果后面想要做精确的同步，则动画同步也是必须的事。
    // 倒是普通的受击、死亡之类的，直接修改一个状态，然后同步给客户端就好了。
    space_service::PlayerAnimation player_animation;
    player_animation.set_name(get_name());

    space_service::Animation* animation = player_animation.mutable_data();
    animation->set_name(combo_animations[combo_seq]);
    // TODO 属性实现
    animation->set_speed(1.f);
    animation->set_op(space_service::Animation::OperationType::Animation_OperationType_START);

    std::string msg_bytes;
    player_animation.SerializeToString(&msg_bytes);
    _space->call_others(this, "sync_animation", msg_bytes);

    // TODO 普攻效果需要读配置，目前先暂定为0.5秒后对处于面前2米60度扇形区域内的敌人造成10点伤害
    G_Timer.add_timer(500, [this]() {
        // FIXME 指针安全
        Vector3f center = get_position();
        Rotation rot = get_rotation();
        float ux = sinf(rot.yaw * DEG2RAD);
        float uz = cosf(rot.yaw * DEG2RAD);

        std::vector<Player*> others = _space->find_players_in_sector(center.x, center.z, ux, uz, 2.f, 30.f * DEG2RAD);
        for (Player* other : others) {
            if (other == this)
                continue;
            other->take_damage(this, 10);
        }
    });
}

void Player::skill_attack(int skill_id)
{
    space_service::PlayerAnimation player_animation;
    player_animation.set_name(get_name());

    space_service::Animation* animation = player_animation.mutable_data();
    animation->set_name("Skill1");
    // TODO 属性实现
    animation->set_speed(1.f);
    animation->set_op(space_service::Animation::OperationType::Animation_OperationType_START);

    std::string msg_bytes;
    player_animation.SerializeToString(&msg_bytes);
    _space->call_others(this, "sync_animation", msg_bytes);

    G_Timer.add_timer(1500, [this]() {
        // FIXME 指针安全
        Vector3f center = get_position();

        std::vector<Player*> others = _space->find_players_in_circle(center.x, center.z, 2.f);
        for (Player* other : others) {
            if (other == this)
                continue;
            other->take_damage(this, 10);
        }
    });
}

void Player::take_damage(Player* attacker, int damage)
{
    // TODO calc real damage
    space_service::TakeDamage msg;
    msg.set_name(_name);
    msg.set_damage(damage);

    std::string msg_bytes;
    msg.SerializeToString(&msg_bytes);
    _space->call_all("take_damage", msg_bytes);
}
