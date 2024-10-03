#pragma once

#include <string>

class TcpConnection;

class Player {
public:
    // conn的生命周期由外部管理
    Player(TcpConnection* conn, const std::string& name) : _conn(conn), _name(name) {}
    ~Player() {}

    inline TcpConnection* get_conn() { return _conn; }
    inline const std::string& get_name() const { return _name; }

    void send_msg(const char* msg_bytes, size_t n);

    inline void set_position(float x, float y, float z) {
        _x = x;
        _y = y;
        _z = z;
    }

    inline void get_position(float& x, float& y, float& z) const {
        x = _x;
        y = _y;
        z = _z;
    }

private:
    TcpConnection* _conn;
    std::string _name;

    float _x;
    float _y;
    float _z;
};
