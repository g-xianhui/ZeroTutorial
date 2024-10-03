#include "player.h"
#include "network/tcp_connection.h"

void Player::send_msg(const char* msg_bytes, size_t n)
{
    if (_conn)
        _conn->send_msg(msg_bytes, n);
}
