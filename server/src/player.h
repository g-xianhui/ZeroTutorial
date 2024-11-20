#pragma once

#include <string>

class TcpConnection;

struct Vector3f {
    float x = 0;
    float y = 0;
    float z= 0;
};

struct Rotation {
    float pitch = 0;
    float yaw = 0;
    float roll = 0;
};

class Player {
public:
    // conn的生命周期由外部管理
    Player(TcpConnection* conn, const std::string& name) : _conn(conn), _name(name) {}
    ~Player() {}

    inline TcpConnection* get_conn() { return _conn; }
    inline const std::string& get_name() const { return _name; }

    void send_msg(const char* msg_bytes, size_t n);

    inline void set_position(float x, float y, float z) {
        _position.x = x;
        _position.y = y;
        _position.z = z;
    }

    inline void get_position(float& x, float& y, float& z) const {
        x = _position.x;
        y = _position.y;
        z = _position.z;
    }

    inline Vector3f get_position() {
        return _position;
    }

    inline void set_rotation(float x, float y, float z) {
        _rotation.pitch = x;
        _rotation.yaw = y;
        _rotation.roll = z;
    }

    inline Rotation get_rotation() {
        return _rotation;
    }

    inline void set_velocity(float x, float y, float z) {
        _velocity.x = x;
        _velocity.y = y;
        _velocity.z = z;
    }

    inline Vector3f get_velocity() {
        return _velocity;
    }

    inline void set_pos_update(bool is_update) { _is_pos_update = is_update; }
    inline bool is_pos_update() { return _is_pos_update; }

private:
    TcpConnection* _conn;
    std::string _name;

    Vector3f _position;
    Rotation _rotation;
    Vector3f _velocity;

    bool _is_pos_update = false;
};
