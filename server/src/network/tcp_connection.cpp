#include "network/tcp_connection.h"

#include <event2/buffer.h>

#include <iostream>

const size_t snd_buf_limit = 64 * 1024;

static void
socket_read_cb(struct bufferevent* bev_, void* ctx)
{
    TcpConnection* conn = (TcpConnection*)ctx;
    char buffer[2048] = { 0 };

    while (1) {
        size_t n = bufferevent_read(bev_, buffer, sizeof(buffer));
        if (n <= 0)
            break;
        conn->handle_data(buffer, n);
    }
}

static void
socket_event_cb(struct bufferevent* bev_, short events, void* ctx)
{
    TcpConnection* conn = (TcpConnection*)ctx;
    if (events & BEV_EVENT_ERROR) {
        int err = EVUTIL_SOCKET_ERROR();
        conn->on_error(err);
    }

    if (events & BEV_EVENT_EOF) {
        conn->on_peer_close();
    }
}

TcpConnection::TcpConnection(struct event_base* base, evutil_socket_t fd)
{
    bev_ = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev_, socket_read_cb, NULL, socket_event_cb, (void*)this);
    bufferevent_enable(bev_, EV_READ | EV_WRITE);
}

TcpConnection::~TcpConnection()
{
    if (bev_)
        bufferevent_free(bev_);
}

void TcpConnection::handle_data(const char* buffer, size_t n)
{
    send_data(buffer, n);
}

void TcpConnection::send_data(const char* data, size_t n)
{
    struct evbuffer* buf = bufferevent_get_output(bev_);
    size_t len = evbuffer_get_length(buf);
    if (len + n > snd_buf_limit) {
        std::cerr << __FUNCTION__ << " reach snd buf limit, write " << n << ", cur_len: " << len << std::endl;
        on_lost_connection();
    }
    else {
        bufferevent_write(bev_, data, n);
    }
}

void TcpConnection::on_peer_close()
{
    on_lost_connection();
}

void TcpConnection::on_error(int err)
{
#ifdef _WIN32
    if (err == WSAECONNRESET) {
        on_lost_connection();
        return;
    }
#else
    if (err == EINTR || err == EWOULDBLOCK)
        return;

    if (err == ECONNRESET || err == EPIPE) {
        on_lost_connection();
    }
#endif
}

void TcpConnection::on_lost_connection()
{
    std::cerr << __FUNCTION__ << std::endl;
    // actually on_lost_connection means half close, we have to close the connection on our side
    bufferevent_free(bev_);
    bev_ = nullptr;

    if (on_lost_connection_callback_)
        on_lost_connection_callback_();
}
