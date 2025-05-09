#include "player.h"
#include "space.h"
#include "wheel_timer.h"
#include "math_utils.h"
#include "bit_stream.h"

#include "network/tcp_connection.h"
#include "proto/space_service.pb.h"

#include "combat/combat_component.h"
#include "movement_component.h"
#include "space_component.h"
#include "connection_component.h"

Player::Player(TcpConnection* conn, const std::string& name) : Entity(), _name(name)
{
    add_component<MovementComponent>();
    add_component<SpaceComponent>();
    add_component<CombatComponent>();
    add_component<ConnectionComponent>(conn);
}

Player::~Player()
{
}

void Player::net_serialize(OutputBitStream& bs, bool to_self) const
{
    bs.write(_name);
}

bool Player::net_delta_serialize(OutputBitStream& bs, bool to_self)
{
    bool dirty = false;

    bs.write(_dirty_flag);
    if (_dirty_flag) {
        dirty = true;

        WRITE_IF_DIRTY(name);
    }

    return dirty;
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

    SpaceComponent* space_comp = get_component<SpaceComponent>();
    Space* space = space_comp->get_space();
    assert(space);
    if (sync_to_all)
        space->call_all(get_eid(), "sync_animation", msg_bytes);
    else
        space->call_others(get_eid(), "sync_animation", msg_bytes);
}
