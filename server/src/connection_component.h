#pragma once

#include "icomponent.h"
#include "network/tcp_connection.h"

#include <string_view>

class TcpConnection;

class ConnectionComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "ConnectionComponent";

    ConnectionComponent(Entity* entity, TcpConnection* conn) : IComponent(entity), _conn(conn) {}

    inline TcpConnection* get_conn() { return _conn; }

    inline void send_msg(const char* msg_bytes, size_t n) {
        if (_conn)
            _conn->send_msg(msg_bytes, n);
    }
private:
    TcpConnection* _conn;
};
