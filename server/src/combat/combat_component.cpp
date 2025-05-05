#include "combat/combat_component.h"
#include "combat/skill/skill_factory.h"

#include "player.h"
#include "space.h"
#include "math_utils.h"
#include "wheel_timer.h"
#include "bit_stream.h"

#include "network/tcp_connection.h"
#include "proto/space_service.pb.h"

#include <spdlog/spdlog.h>

constexpr const char* combo_animations[] = { "Attack01", "Attack02" };

void CombatComponent::start()
{
    _attr_set.init();

    {
        SkillInfo skill_info{
            .skill_id = 1,
            .anmimator_state = "Skill1",
            .cost_mana = 2,
            .cool_down = 5000
        };
        _skill_infos.push_back(skill_info);
    }
}

void CombatComponent::stop()
{
    if (_normal_attack_timer != -1)
    {
        G_Timer.del_timer(_normal_attack_timer);
        _normal_attack_timer = -1;
    }
    stop_running_skill();
}

void CombatComponent::net_serialize(OutputBitStream& bs)
{
    _attr_set.net_serialize(bs);
}

bool CombatComponent::consume_dirty(OutputBitStream& bs)
{
    bool dirty = false;
    dirty |= _attr_set.consume_dirty(bs);
    return dirty;
}

void CombatComponent::normal_attack(int combo_seq)
{
    if (combo_seq < 0 || combo_seq >= sizeof(combo_animations) / sizeof(const char*))
        return;

    size_t now = G_Timer.ms_since_start();
    if (now < _next_normal_attack_time) {
        spdlog::error("normal attack cd limit, _next_normal_attack_time: {}, now: {}", _next_normal_attack_time, now);
        return;
    }

    float interval = 1.0f / (_attr_set.attack_speed + _attr_set.additional_attack_speed);
    _next_normal_attack_time = now + static_cast<size_t>(interval * 1000);
    
    // 这里直接告知客户端播放某个动画，而不是告知客户端执行normal_attack，原因是普攻（包括后面的技能）可能做得很复杂，有多种效果，随机或玩家选择，
    // 因此直接让服务器来算这个逻辑，告知客户端结果算了。如果动画很长，可以把animation的基础信息记录下来，新玩家上线时同步过去就是了。
    // 另一方面，技能动画可能是带root motion的，如果后面想要做精确的同步，则动画同步也是必须的事。
    // 倒是普通的受击、死亡之类的，直接修改一个状态，然后同步给客户端就好了。
    float play_rate = (_attr_set.attack_speed + _attr_set.additional_attack_speed) / _attr_set.attack_speed;
    _owner->play_animation(combo_animations[combo_seq], play_rate);

    // TODO 普攻效果暂定为: 0.5秒后对处于面前2米60度扇形区域内的敌人造成10点伤害
    _normal_attack_timer = G_Timer.add_timer(500, [this]() {
        // FIXME 指针安全
        Vector3f center = _owner->get_position();
        Rotation rot = _owner->get_rotation();
        float ux = sinf(rot.yaw * DEG2RAD);
        float uz = cosf(rot.yaw * DEG2RAD);

        Space* space = _owner->get_space();

        std::vector<Player*> others = space->find_players_in_sector(center.x, center.z, ux, uz, 2.f, 30.f * DEG2RAD);
        for (Player* other : others) {
            if (other == _owner)
                continue;

            CombatComponent* comp = other->get_component<CombatComponent>();
            if (comp)
                comp->take_damage(this, 10);
        }

        _normal_attack_timer = -1;
    });
}

void CombatComponent::cast_skill(int skill_id)
{
    auto iter = std::find_if(_skill_infos.begin(), _skill_infos.end(), [skill_id](const SkillInfo& info) {
        return info.skill_id == skill_id;
    });
    if (iter == _skill_infos.end()) {
        spdlog::error("cast skill but skill {} not found!", skill_id);
        return;
    }

    SkillInfo& info = *iter;
    if (can_cast_skill(info)) {
        // reduce cost
        _attr_set.add_mana(-info.cost_mana);
        info.next_cast_time = int(G_Timer.ms_since_start()) + info.cool_down;

        ISkill* skill = get_or_create_skill_instance(skill_id, info.instance_per_entity);
        skill->execute();
    }
    else {
        // 强制更新客户端蓝量
        _attr_set.add_mana(0);
    }

    // 更新客户端cd
    // TODO 动态数组目前还需要自己去rpc同步变化
    sync_skill_info(info);
}

bool CombatComponent::can_cast_skill(const SkillInfo& info)
{
    if (_attr_set.get_mana() < info.cost_mana)
        return false;

    if (G_Timer.ms_since_start() < info.next_cast_time)
        return false;

    if (_running_skills.contains(info.skill_id)) {
        ISkill* skill = _running_skills[info.skill_id];
        assert(skill);
        if (skill->is_active())
            return false;
    }

    return true;
}

ISkill* CombatComponent::get_or_create_skill_instance(int skill_id, bool instance_per_entity)
{
    if (_running_skills.contains(skill_id)) {
        ISkill* skill = _running_skills[skill_id];
        assert(skill && !skill->is_active());

        if (instance_per_entity) {
            skill->reset();
        }
        else {
            delete skill;
            skill = create_skill_instance(skill_id);
        }
        return skill;
    }
    else {
        return create_skill_instance(skill_id);
    }
}

ISkill* CombatComponent::create_skill_instance(int skill_id)
{
    ISkill* skill = SkillFactory::instance().create(skill_id);
    assert(skill);
    skill->set_owner(this);
    _running_skills[skill_id] = skill;
    return skill;
}

void CombatComponent::stop_running_skill()
{
    for (auto [_, skill] : _running_skills) {
        skill->destroy();
        delete skill;
    }
    _running_skills.clear();
}

void CombatComponent::take_damage(CombatComponent* attacker, int damage)
{
    damage = _attr_set.take_damage(attacker->_attr_set, damage);
    if (damage <= 0)
        return;

    space_service::TakeDamage msg;
    msg.set_eid(_owner->get_eid());
    msg.set_damage(damage);

    std::string msg_bytes;
    msg.SerializeToString(&msg_bytes);
    Space* space = _owner->get_space();
    space->call_all(_owner->get_eid(), "take_damage", msg_bytes);
}

void CombatComponent::sync_skill_info(const SkillInfo& skill_info)
{
    space_service::SkillInfo msg;
    msg.set_skill_id(skill_info.skill_id);
    msg.set_next_cast_time(skill_info.next_cast_time);
    send_proto_msg(_owner->get_conn(), "update_skill_info", msg);
}
