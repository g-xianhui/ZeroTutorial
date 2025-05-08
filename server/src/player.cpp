#include "player.h"
#include "space.h"
#include "wheel_timer.h"
#include "math_utils.h"
#include "bit_stream.h"

#include "network/tcp_connection.h"
#include "proto/space_service.pb.h"

#include "combat/combat_component.h"
#include "combat/skill/skill_factory.h"

static int gen_eid() {
    static int G_IDX = 0;
    G_IDX += 1;
    return G_IDX;
}

Player::Player(TcpConnection* conn, const std::string& name) : _conn(conn), _name(name)
{
    set_eid(gen_eid());
    start();
}

Player::~Player()
{
    stop();
}

void Player::start()
{
    add_component<CombatComponent>();

    for (auto [_, comp] : _components) {
        comp->start();
    }
}

void Player::stop()
{
    for (auto [_, comp] : _components) {
        comp->stop();
    }

    _components.clear();
}

void Player::net_serialize(OutputBitStream& bs) const
{
    bs.write(_eid);
    bs.write(_name);
    for (auto& [name, comp] : _components) {
        bs.write(name);
        comp->net_serialize(bs);
    }
}

bool Player::net_delta_serialize(OutputBitStream& bs)
{
    bool dirty = false;

    bs.write(_dirty_flag);
    if (_dirty_flag) {
        dirty = true;

        WRITE_IF_DIRTY(name);

        _dirty_flag = 0;
    }

    for (auto& [name, comp] : _components) {
        bs.write(name);
        dirty |= comp->net_delta_serialize(bs);
    }
    return dirty;
}

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

template<IsComponent T>
void Player::add_component()
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    assert(iter == _components.end());

    IComponent* comp = new T();
    comp->set_owner(this);
    _components[name] = comp;
}

template<IsComponent T>
void Player::remove_component()
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    if (iter != _components.end()) {
        IComponent* comp = iter->second;
        comp->stop();

        _components.erase(iter);
    }
}

template<IsComponent T>
T* Player::get_component()
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    return iter == _components.end() ? nullptr : static_cast<T*>(iter->second);
}

void Player::set_position(float x, float y, float z) {
    _position.x = x;
    _position.y = y;
    _position.z = z;

    if (_space) {
        _space->update_position(get_eid(), x, y, z);
    }
}

constexpr const char* combo_animations[] = { "Attack01", "Attack02" };

void Player::normal_attack(int combo_seq)
{
    CombatComponent* comp = get_component<CombatComponent>();
    if (comp != nullptr) {
        comp->normal_attack(combo_seq);
    }
}

void Player::skill_attack(int skill_id)
{
    CombatComponent* comp = get_component<CombatComponent>();
    if (comp != nullptr) {
        comp->cast_skill(skill_id);
    }
}

void Player::play_animation(const std::string& name, float speed, bool sync_to_all)
{
    // TODO 最好弄个属性同步机制，新上线的玩家也能看到
    space_service::PlayerAnimation player_animation;
    player_animation.set_eid(get_eid());

    space_service::Animation* animation = player_animation.mutable_data();
    animation->set_name(name);
    animation->set_speed(speed);
    animation->set_op(space_service::Animation::OperationType::Animation_OperationType_START);

    std::string msg_bytes;
    player_animation.SerializeToString(&msg_bytes);
    if (sync_to_all)
        _space->call_all(get_eid(), "sync_animation", msg_bytes);
    else
        _space->call_others(get_eid(), "sync_animation", msg_bytes);
}
