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

    void on_new_connection(struct evconnlistener* listener, evutil_socket_t fd);

private:
    std::string _host;
    int _port;
    // 提供的服务
    Service* _service;

    struct evconnlistener* _listener = nullptr;
    struct evconnlistener* _listener_v6 = nullptr;
};

