#pragma once

#include "icomponent.h"

#include <string_view>

class Space;

class SpaceComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "SpaceComponent";

    SpaceComponent(Entity* entity) : IComponent(entity) {}

    void enter_space(Space* space) { _space = space; }
    void leave_space() { _space = nullptr; }
    inline Space* get_space() { return _space; }

private:
    Space* _space = nullptr;
};