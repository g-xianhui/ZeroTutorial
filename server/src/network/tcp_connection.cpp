#include "network/tcp_connection.h"

#include "service.h"

#include "bit_utils.h"

#include <event2/buffer.h>

#include <iostream>
#include <cassert>

const size_t SEND_BUFFER_SIZE = 16 * 1024 * 1024;

std::vector<std::string> RecvBuffer::recv(const char* bytes, size_t n) {
    std::vector<std::string> msgs;

    size_t bindex = 0;
    while (_need_bytes > 0 && bindex < n)
    {
        switch (_parse_stage)
        {
        case ParseStage::LEN:
            _len_bytes[_len_bytes_position++] = bytes[bindex];
            if ((bytes[bindex] & 0x80) == 0)
                _need_bytes = 0;

            bindex += 1;
            if (_need_bytes == 0)
            {
                _parse_stage = ParseStage::DATA;
                _need_bytes = calc_package_data_length();
                _len_bytes_position = 0;

                assert(_need_bytes <= MAX_PACKAGE_SIZE);
                // FIXME too slow
                _buffer = new char[_need_bytes];
            }
            break;
        case ParseStage::DATA:
            size_t leftBytesNum = n - bindex;
            if (leftBytesNum < _need_bytes)
            {
                memcpy(_buffer + _position, bytes + bindex, leftBytesNum);
                _need_bytes -= leftBytesNum;
                bindex += leftBytesNum;
                _position += leftBytesNum;
            }
            else
            {
                memcpy(_buffer + _position, bytes + bindex, _need_bytes);
                bindex += _need_bytes;

                // finish one msg
                std::string msg{ _buffer, _need_bytes };
                msgs.push_back(std::move(msg));

                // reset to initial state
                _parse_stage = ParseStage::LEN;
                _need_bytes = 1;

                delete[] _buffer;
                _buffer = nullptr;
                _position = 0;
            }
            break;
        }
    }

    return msgs;
}

size_t RecvBuffer::calc_package_data_length()
{
    return read_7bit_encoded_int(_len_bytes);
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

TcpConnection::TcpConnection(struct event_base* base, evutil_socket_t fd, Service* service) : _service(service)
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
        _service->handle_msg(this, msg);
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
    char len_bytes[5] = { 0 };
    size_t encoded_size = write_7bit_encoded_int((uint32_t)n, len_bytes);
    send_data(len_bytes, encoded_size);
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

    _service->on_lost_connection(this);
}
