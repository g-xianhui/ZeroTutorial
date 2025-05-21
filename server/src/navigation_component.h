#pragma once

#include "icomponent.h"
#include "math_utils.h"

#include <vector>

class NavigationComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "NavigationComponent";

    NavigationComponent(Entity* entity) : IComponent(entity) {}

    void move_to(const Vector3f& pos);

    void update(float delta_t);

    inline std::vector<Vector3f> get_path() { return _path; }

private:
    void reset();

private:
    int _update_timer = -1;

    float _move_speed = 2.f;

    float _cumulate_t = 0.f;
    float _segment_t = 0.f;
    size_t _cur_path_idx = 0;
    std::vector<Vector3f> _path;
};