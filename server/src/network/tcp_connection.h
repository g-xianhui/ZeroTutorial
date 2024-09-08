#pragma once

#include <event2/event.h>
#include <event2/bufferevent.h>

#include <functional>
#include <vector>
#include <string>

class RecvBuffer {
public:
    enum class ParseStage {
        TLEN,
        DATA
    };

    static const size_t TLEN_SIZE = 2;
    static const size_t MAX_PACKAGE_SIZE = 1 << (TLEN_SIZE * 8);

    RecvBuffer() {
        _buffer = new char[MAX_PACKAGE_SIZE];
    }

    ~RecvBuffer() {
        delete[] _buffer;
    }

    std::vector<std::string> recv(const char* bytes, size_t n);

    inline size_t calc_package_data_length()
    {
        return *(reinterpret_cast<uint16_t*>(_tlenBytes));
    }
private:
    char _tlenBytes[TLEN_SIZE] = { 0 };
    ParseStage _parseStage = ParseStage::TLEN;
    size_t _needBytes = TLEN_SIZE;

    // next useful position of buffer
    size_t _position = 0;
    char* _buffer;
};

class TcpConnection {
public:
    TcpConnection(struct event_base* base, evutil_socket_t fd);
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
    struct bufferevent* _bev = nullptr;
    std::function<void()> _on_lost_connection_callback;

    RecvBuffer _recvBuffer;
};
