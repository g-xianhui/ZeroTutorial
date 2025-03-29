#pragma once

#include <concepts>
#include <string>

class Player;

class IComponent {
public:
    ~IComponent() = default;

    inline void set_owner(Player* p) { _owner = p; }
    inline Player* get_owner() { return _owner; }

    virtual void start() {}
    virtual void stop() {}

protected:
    Player* _owner;
};


template<typename T>
concept IsComponent = std::is_base_of_v<IComponent, T> &&
    requires {
        { T::COMPONENT_NAME } -> std::convertible_to<const char*>;
    };