#pragma once

#include <event2/event.h>
#include <event2/bufferevent.h>

#include <functional>
#include <vector>
#include <string>

class Service;

class RecvBuffer {
public:
    enum class ParseStage {
        LEN,
        DATA
    };

    // uint32 can be encoded as 5 bytes
    static const size_t MAX_LEN_SIZE = 5;
    static const size_t MAX_PACKAGE_SIZE = 16 * 1024 * 1024;

    std::vector<std::string> recv(const char* bytes, size_t n);

    size_t calc_package_data_length();

private:
    char _len_bytes[MAX_LEN_SIZE] = { 0 };

    ParseStage _parse_stage = ParseStage::LEN;
    size_t _len_bytes_position = 0;
    size_t _need_bytes = 1;

    // next useful position of buffer
    size_t _position = 0;
    char* _buffer = nullptr;
};

class TcpConnection {
public:
    TcpConnection(struct event_base* base, evutil_socket_t fd, Service* service);
    ~TcpConnection();

    void handle_data(const char* buffer, size_t n);
    void send_data(const char* data, size_t n);

    void send_msg(const char* msg_bytes, size_t n);

    void on_peer_close();
    void on_error(int err);
    void on_lost_connection();

    inline void set_lost_callback(std::function<void()>&& cb) {
        _on_lost_connection_callback = std::move(cb);
    }

private:
    Service* _service = nullptr;

    struct bufferevent* _bev = nullptr;
    std::function<void()> _on_lost_connection_callback;

    RecvBuffer _recvBuffer;
};
