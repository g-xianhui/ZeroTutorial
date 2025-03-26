#include "player.h"
#include "space.h"
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
