#include "network/tcp_connection.h"

#include <event2/buffer.h>

#include <iostream>
#include <cassert>

const size_t SEND_BUFFER_SIZE = 64 * 1024;

std::vector<std::string> RecvBuffer::recv(const char* bytes, size_t n) {
    std::vector<std::string> msgs;

    size_t bindex = 0;
    while (_needBytes > 0 && bindex < n)
    {
        switch (_parseStage)
        {
        case ParseStage::TLEN:
            _tlenBytes[TLEN_SIZE - _needBytes] = bytes[bindex];
            _needBytes -= 1;
            bindex += 1;
            if (_needBytes == 0)
            {
                _parseStage = ParseStage::DATA;
                _needBytes = calc_package_data_length();

                assert(_needBytes <= MAX_PACKAGE_SIZE);
            }
            break;
        case ParseStage::DATA:
            size_t leftBytesNum = n - bindex;
            if (leftBytesNum < _needBytes)
            {
                memcpy(_buffer + _position, bytes + bindex, leftBytesNum);
                _needBytes -= leftBytesNum;
                bindex += leftBytesNum;
                _position += leftBytesNum;
            }
            else
            {
                memcpy(_buffer + _position, bytes + bindex, _needBytes);
                bindex += _needBytes;
                _position = 0;

                // finish one msg
                std::string msg{ _buffer, _needBytes };
                msgs.push_back(std::move(msg));

                // reset to initial state
                _parseStage = ParseStage::TLEN;
                _needBytes = TLEN_SIZE;
            }
            break;
        }
    }

    return msgs;
}

static void
socket_read_cb(struct bufferevent* _bev, void* ctx)
{
    TcpConnection* conn = (TcpConnection*)ctx;
    char buffer[2048] = { 0 };

    while (1) {
        size_t n = bufferevent_read(_bev, buffer, sizeof(buffer));
        if (n <= 0)
            break;
        conn->handle_data(buffer, n);
    }
}

static void
socket_event_cb(struct bufferevent* _bev, short events, void* ctx)
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
    _bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(_bev, socket_read_cb, NULL, socket_event_cb, (void*)this);
    bufferevent_enable(_bev, EV_READ | EV_WRITE);
}

TcpConnection::~TcpConnection()
{
    if (_bev)
        bufferevent_free(_bev);
}

void TcpConnection::handle_data(const char* buffer, size_t n)
{
    std::vector<std::string> msgs = _recvBuffer.recv(buffer, n);
    for (std::string& msg : msgs) {
        std::cout << "recv: " << msg << std::endl;
        send_msg(msg.c_str(), msg.size());
    }
}

void TcpConnection::send_data(const char* data, size_t n)
{
    struct evbuffer* buf = bufferevent_get_output(_bev);
    size_t len = evbuffer_get_length(buf);
    if (len + n > SEND_BUFFER_SIZE) {
        std::cerr << __FUNCTION__ << " reach snd buf limit, write " << n << ", cur_len: " << len << std::endl;
        on_lost_connection();
    }
    else {
        bufferevent_write(_bev, data, n);
    }
}

void TcpConnection::send_msg(const char* msg_bytes, size_t n)
{
    send_data((const char*)&n, RecvBuffer::TLEN_SIZE);
    send_data(msg_bytes, n);
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
    bufferevent_free(_bev);
    _bev = nullptr;

    if (_on_lost_connection_callback)
        _on_lost_connection_callback();
}
