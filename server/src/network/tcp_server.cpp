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
    struct sockaddr_in sin;

    /* Clear the sockaddr before using it, in case there are extra
    * platform-specific fields that can mess us up. */
    std::memset(&sin, 0, sizeof(sin));
    /* This is an INET address */
    sin.sin_family = AF_INET; 
    /* Listen on 0.0.0.0 */
    sin.sin_addr.s_addr = htonl(0);
    /* Listen on the given port. */
    sin.sin_port = htons(_port);

    _listener = evconnlistener_new_bind(base, accept_conn_cb, this,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
        (struct sockaddr*)&sin, sizeof(sin));
    if (!_listener) {
        spdlog::error("Couldn't create listener");
        return;
    }
    evconnlistener_set_error_cb(_listener, accept_error_cb);
}

void TcpServer::stop()
{
    if (_listener) {
        evconnlistener_disable(_listener);
        evconnlistener_free(_listener);
    }
}

void TcpServer::on_new_connection(struct evconnlistener* listener, evutil_socket_t fd)
{
    struct event_base* base = evconnlistener_get_base(listener);
    TcpConnection* conn = new TcpConnection{base, fd, _service};
    _service->on_new_connection(conn);
}
