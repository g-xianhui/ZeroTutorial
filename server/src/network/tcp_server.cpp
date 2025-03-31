#include "network/tcp_server.h"
#include "network/tcp_connection.h"
#include "service.h"

#include <cstring>
#include <cassert>
#include <spdlog/spdlog.h>

#include <event2/buffer.h>

static void
accept_conn_cb(struct evconnlistener* listener,
    evutil_socket_t fd, struct sockaddr* address, int socklen,
    void* ctx)
{
    TcpServer* tcp_server = (TcpServer*)ctx;
    tcp_server->on_new_connection(listener, fd);
}

static void
accept_error_cb(struct evconnlistener* listener, void* ctx)
{
    struct event_base* base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    spdlog::error("Got an error {} ({}) on the listener.\n Shutting down.",
        err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}

void TcpServer::start(struct event_base* base)
{
    struct sockaddr_in addr_v4 {};
    addr_v4.sin_family = AF_INET;
    addr_v4.sin_addr.s_addr = INADDR_ANY;
    addr_v4.sin_port = htons(_port);

    _listener = evconnlistener_new_bind(base, accept_conn_cb, this,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
        (struct sockaddr*)&addr_v4, sizeof(addr_v4));
    if (!_listener) {
        spdlog::error("Couldn't create listener for ipv4");
        return;
    }
    evconnlistener_set_error_cb(_listener, accept_error_cb);

    sockaddr_in6 addr_v6{};
    addr_v6.sin6_family = AF_INET6;
    addr_v6.sin6_addr = in6addr_any;
    addr_v6.sin6_port = htons(_port);

    _listener_v6 = evconnlistener_new_bind(base, accept_conn_cb, this,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
        (struct sockaddr*)&addr_v6, sizeof(addr_v6));
    if (!_listener_v6) {
        spdlog::error("Couldn't create listener for ipv6");
        return;
    }
    evconnlistener_set_error_cb(_listener_v6, accept_error_cb);
}

void TcpServer::stop()
{
    if (_listener) {
        evconnlistener_disable(_listener);
        evconnlistener_free(_listener);
    }

    if (_listener_v6) {
        evconnlistener_disable(_listener_v6);
        evconnlistener_free(_listener_v6);
    }
}

void TcpServer::on_new_connection(struct evconnlistener* listener, evutil_socket_t fd)
{
    struct event_base* base = evconnlistener_get_base(listener);
    TcpConnection* conn = new TcpConnection{base, fd, _service};
    _service->on_new_connection(conn);
}
