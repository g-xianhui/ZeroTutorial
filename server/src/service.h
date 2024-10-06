#pragma once

#include "globals.h"
#include "network/tcp_server.h"
#include "network/tcp_connection.h"

#include <string>

class Service {
public:
    virtual ~Service() {}

    virtual void start(const std::string& listen_address, int listen_port) {
        _server = new TcpServer(listen_address, listen_port, this);
        _server->start(EVENT_BASE);
    }

    virtual void stop() {
        _server->stop();
        _server = nullptr;
    }

    virtual void on_new_connection(TcpConnection* conn) {}

    virtual void on_lost_connection(TcpConnection* conn) {
        // 默认直接把连接对象删掉
        delete conn;
    }

    virtual void handle_msg(TcpConnection* conn, const std::string& msg) = 0;

private:
    TcpServer* _server = nullptr;
};
