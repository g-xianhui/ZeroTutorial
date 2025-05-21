#include "navigation_component.h"
#include "movement_component.h"
#include "space_component.h"

#include "entity.h"
#include "space.h"

#include "wheel_timer.h"

void NavigationComponent::move_to(const Vector3f& pos)
{
    MovementComponent* movement_comp = get_owner()->get_component<MovementComponent>();
    assert(movement_comp);

    SpaceComponent* space_comp = get_owner()->get_component<SpaceComponent>();
    assert(space_comp);

    reset();

    Vector3f start_pos = movement_comp->get_position();
    _path = space_comp->get_space()->find_path(start_pos, pos);

    if (_path.empty())
        return;

    if (_path.size() == 1) {
        movement_comp->look_at(_path[0]);
        movement_comp->set_position(_path[0].x, _path[0].y, _path[0].z);
        return;
    }

    _update_timer = G_Timer.add_timer(33, [this]() {
        this->update(0.033f);
    }, true);
}

void NavigationComponent::update(float delta_t)
{
    MovementComponent* movement_comp = get_owner()->get_component<MovementComponent>();

    Vector3f& start_pos = _path[_cur_path_idx];
    Vector3f& end_pos = _path[_cur_path_idx + 1];

    if (_cumulate_t == 0) {
        Vector3f dir = end_pos - start_pos;
        _segment_t = dir.lenght() / _move_speed;
        movement_comp->look_at(end_pos);
    }

    _cumulate_t += delta_t;

    if (_cumulate_t >= _segment_t) {
        if (_cur_path_idx >= _path.size() - 2) {
            Vector3f pos = end_pos;
            movement_comp->set_position(pos.x, pos.y, pos.z);
            movement_comp->set_move_timestamp(G_Timer.ms_since_start() / 1000.f);

            reset();
        }
        else {
            float new_delta_t = _cumulate_t - _segment_t;
            _segment_t = 0;
            _cumulate_t = 0;
            _cur_path_idx += 1;

            update(new_delta_t);
        }
    }
    else {
        float factor = _cumulate_t / _segment_t;

        Vector3f pos = (1.f - factor) * start_pos + factor * end_pos;

        movement_comp->set_position(pos.x, pos.y, pos.z);
        movement_comp->set_move_timestamp(G_Timer.ms_since_start() / 1000.f);
    }
}

void NavigationComponent::reset()
{
    if (_update_timer != -1) {
        G_Timer.del_timer(_update_timer);
        _update_timer = -1;
    }

    _cur_path_idx = 0;
    _cumulate_t = 0;
    _segment_t = 0;
    _path.clear();
}
