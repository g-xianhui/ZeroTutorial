#pragma once

#include <map>
#include <string>

#include <event2/listener.h>
#include <event2/bufferevent.h>

class TcpConnection;

class TcpServer
{
public:
    TcpServer(const std::string& host, int port) : host_(host), port_(port) {}

    void start(struct event_base* base);
    void stop();

    void kill_connection(int conn);

    int on_new_connection(struct evconnlistener* listener, evutil_socket_t fd);
    void on_lost_connection(int conn);

private:
    std::string host_;
    int port_;

    struct evconnlistener* listener_ = nullptr;
    int next_conn_id_ = 0;
    std::map<int, TcpConnection*> id_2_conn_;
};

