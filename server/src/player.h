#pragma once

#include <string>
#include <unordered_map>

#include "icomponent.h"

class TcpConnection;

struct Vector3f {
    float x = 0;
    float y = 0;
    float z = 0;
};

struct Rotation {
    float pitch = 0;
    float yaw = 0;
    float roll = 0;
};

class Space;

class Player {
public:
    // conn的生命周期由外部管理
    Player(TcpConnection* conn, const std::string& name);
    ~Player();

    void start();
    void stop();

    inline TcpConnection* get_conn() { return _conn; }
    inline const std::string& get_name() const { return _name; }

    void send_msg(const char* msg_bytes, size_t n);

    void enter_space(Space* space);
    void leave_space();
    inline Space* get_space() { return _space; }

    template<IsComponent T>
    void add_component();

    template<IsComponent T>
    void remove_component();

    template<IsComponent T>
    T* get_component();

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

    inline void set_acceleration(float x, float y, float z) {
        _acceleration.x = x;
        _acceleration.y = y;
        _acceleration.z = z;
    }

    inline Vector3f get_acceleration() {
        return _acceleration;
    }

    inline void set_angular_velocity(float x, float y, float z) {
        _angular_velocity.x = x;
        _angular_velocity.y = y;
        _angular_velocity.z = z;
    }

    inline Vector3f get_angular_velocity() {
        return _angular_velocity;
    }

    inline void set_move_mode(int mode) {
        _move_mode = mode;
    }

    inline int get_move_mode() {
        return _move_mode;
    }

    inline void set_move_timestamp(float t) {
        _move_timestamp = t;
    }

    inline float get_move_timestamp() {
        return _move_timestamp;
    }

    inline void set_pos_update(bool is_update) { _is_pos_update = is_update; }
    inline bool is_pos_update() { return _is_pos_update; }

    void normal_attack(int combo_seq);
    void skill_attack(int skill_id);

    void play_animation(const std::string& name, float speed=1.f);

private:
    TcpConnection* _conn;
    std::string _name;

    Space* _space = nullptr;

    std::unordered_map<std::string, IComponent*> _components;

    Vector3f _position;
    Rotation _rotation;
    Vector3f _velocity;
    Vector3f _acceleration;
    Vector3f _angular_velocity;
    int _move_mode;
    float _move_timestamp;

    bool _is_pos_update = false;
};
