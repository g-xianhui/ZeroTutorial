#pragma once

#include "service.h"

#include "network/tcp_connection.h"

class EchoService : public Service {
public:
    virtual void handle_msg(TcpConnection* conn, const std::string& msg) override {
        conn->send_msg(msg.c_str(), msg.size());
    }
};
