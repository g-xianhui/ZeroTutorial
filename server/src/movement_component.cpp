#include "movement_component.h"
#include "space_component.h"
#include "space.h"
#include "entity.h"

#include "bit_stream.h"

#include <spdlog/spdlog.h>

void MovementComponent::net_serialize(OutputBitStream& bs, bool to_self) const
{
    if (to_self)
        return;

    space_service::Movement new_move;
    fill_movement_data(&new_move);

    std::string msg_bytes;
    new_move.SerializeToString(&msg_bytes);

    bs.write(msg_bytes);
}

bool MovementComponent::net_delta_serialize(OutputBitStream& bs, bool to_self)
{
    if (to_self)
        return false;

    net_serialize(bs, to_self);
    return true;
}

void MovementComponent::set_position(float x, float y, float z)
{
    _position.x = x;
    _position.y = y;
    _position.z = z;

    SpaceComponent* space_comp = _owner->get_component<SpaceComponent>();
    if (space_comp) {
        Space* space = space_comp->get_space();
        if (space)
            space->update_position(_owner->get_eid(), x, y, z);
    }
}

void MovementComponent::look_at(const Vector3f& pos)
{
    Vector3f dir = pos - _position;
    dir.y = 0;

    float yaw = std::atan2(dir.x, dir.z) / DEG2RAD;
    Rotation r{ 0, yaw, 0 };

    spdlog::debug("dir: ({}, {}, {}), rotation: ({}, {}, {})", dir.x, dir.y, dir.z, r.pitch, r.yaw, r.roll);
    set_rotation(r);
}

void MovementComponent::fill_movement_data(space_service::Movement* new_move) const
{
    Vector3f cur_position = get_position();
    Rotation cur_rotation = get_rotation();
    Vector3f cur_velocity = get_velocity();
    Vector3f cur_acceleration = get_acceleration();
    Vector3f cur_angular_velocity = get_angular_velocity();

    space_service::Vector3f* position = new_move->mutable_position();
    position->set_x(cur_position.x);
    position->set_y(cur_position.y);
    position->set_z(cur_position.z);

    space_service::Vector3f* rotation = new_move->mutable_rotation();
    rotation->set_x(cur_rotation.pitch);
    rotation->set_y(cur_rotation.yaw);
    rotation->set_z(cur_rotation.roll);

    space_service::Vector3f* velocity = new_move->mutable_velocity();
    velocity->set_x(cur_velocity.x);
    velocity->set_y(cur_velocity.y);
    velocity->set_z(cur_velocity.z);

    space_service::Vector3f* acceleration = new_move->mutable_acceleration();
    acceleration->set_x(cur_acceleration.x);
    acceleration->set_y(cur_acceleration.y);
    acceleration->set_z(cur_acceleration.z);

    space_service::Vector3f* angular_velocity = new_move->mutable_angular_velocity();
    angular_velocity->set_x(cur_angular_velocity.x);
    angular_velocity->set_y(cur_angular_velocity.y);
    angular_velocity->set_z(cur_angular_velocity.z);

    new_move->set_mode(get_move_mode());
    new_move->set_timestamp(get_move_timestamp());
}
