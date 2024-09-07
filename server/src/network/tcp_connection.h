#pragma once

#include <event2/event.h>
#include <event2/bufferevent.h>

#include <functional>

class TcpConnection {
public:
    TcpConnection(struct event_base* base, evutil_socket_t fd);
    ~TcpConnection();

    void handle_data(const char* buffer, size_t n);
    void send_data(const char* data, size_t n);

    void on_peer_close();
    void on_error(int err);
    void on_lost_connection();

    inline void set_lost_callback(std::function<void()>&& cb) {
        on_lost_connection_callback_ = std::move(cb);
    }

private:
    struct bufferevent* bev_ = nullptr;
    std::function<void()> on_lost_connection_callback_;
};
