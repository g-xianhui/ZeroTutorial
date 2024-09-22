#pragma once

#include "globals.h"
#include "network/tcp_server.h"

#include <string>

class TcpConnection;

class Service {
public:
    inline void start(const std::string& listen_address, int listen_port) {
        _server = new TcpServer(listen_address, listen_port, this);
        _server->start(EVENT_BASE);
    }

    inline void stop() {
        _server->stop();
    }

    virtual void handle_msg(TcpConnection* conn, const std::string& msg) = 0;

private:
    TcpServer* _server = nullptr;
};
