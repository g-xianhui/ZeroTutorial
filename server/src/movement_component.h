#pragma once

#include "icomponent.h"
#include "proto/space_service.pb.h"

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

class MovementComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "MovementComponent";

    MovementComponent(Entity* entity) : IComponent(entity) {}

    virtual void net_serialize(OutputBitStream& bs, bool to_self) const override;
    virtual bool net_delta_serialize(OutputBitStream& bs, bool to_self) override;

    void set_position(float x, float y, float z);

    inline void get_position(float& x, float& y, float& z) const {
        x = _position.x;
        y = _position.y;
        z = _position.z;
    }

    inline Vector3f get_position() const {
        return _position;
    }

    inline void set_rotation(float x, float y, float z) {
        _rotation.pitch = x;
        _rotation.yaw = y;
        _rotation.roll = z;
    }

    inline Rotation get_rotation() const {
        return _rotation;
    }

    inline void set_velocity(float x, float y, float z) {
        _velocity.x = x;
        _velocity.y = y;
        _velocity.z = z;
    }

    inline Vector3f get_velocity() const {
        return _velocity;
    }

    inline void set_acceleration(float x, float y, float z) {
        _acceleration.x = x;
        _acceleration.y = y;
        _acceleration.z = z;
    }

    inline Vector3f get_acceleration() const {
        return _acceleration;
    }

    inline void set_angular_velocity(float x, float y, float z) {
        _angular_velocity.x = x;
        _angular_velocity.y = y;
        _angular_velocity.z = z;
    }

    inline Vector3f get_angular_velocity() const {
        return _angular_velocity;
    }

    inline void set_move_mode(int mode) {
        _move_mode = mode;
    }

    inline int get_move_mode() const {
        return _move_mode;
    }

    inline void set_move_timestamp(float t) {
        _move_timestamp = t;
    }

    inline float get_move_timestamp() const {
        return _move_timestamp;
    }

    void fill_movement_data(space_service::Movement* new_move) const;
private:
    Vector3f _position;
    Rotation _rotation;
    Vector3f _velocity;
    Vector3f _acceleration;
    Vector3f _angular_velocity;

    int _move_mode = 0;
    float _move_timestamp = 0.f;
};