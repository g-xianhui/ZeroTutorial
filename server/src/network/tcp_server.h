#pragma once

#include <map>
#include <string>

#include <event2/listener.h>
#include <event2/bufferevent.h>

class Service;
class TcpConnection;

class TcpServer
{
public:
    TcpServer(const std::string& host, int port, Service* service) : _host(host), _port(port), _service(service) {}

    void start(struct event_base* base);
    void stop();

    void kill_connection(int conn);

    int on_new_connection(struct evconnlistener* listener, evutil_socket_t fd);
    void on_lost_connection(int conn);

private:
    std::string _host;
    int _port;
    // 提供的服务
    Service* _service;

    struct evconnlistener* _listener = nullptr;
    int _next_conn_id = 0;
    std::map<int, TcpConnection*> _id_2_conn;
};

