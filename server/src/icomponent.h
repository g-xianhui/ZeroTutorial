#pragma once

#include <concepts>
#include <string>

class Entity;
class OutputBitStream;

class IComponent {
public:
    IComponent(Entity* entity) : _owner(entity) {}
    virtual ~IComponent() {}

    inline void set_owner(Entity* entity) { _owner = entity; }
    inline Entity* get_owner() { return _owner; }

    virtual void init() {}
    virtual void destroy() {}

    virtual void net_serialize(OutputBitStream& bs, bool to_self) const {}
    virtual bool net_delta_serialize(OutputBitStream& bs, bool to_self) { return false; }
    virtual void reset_dirty() { _dirty_flag = 0; }

protected:
    Entity* _owner;
    uint32_t _dirty_flag = 0;
};


template<typename T>
concept IsComponent = std::is_base_of_v<IComponent, T> &&
    requires {
        { T::COMPONENT_NAME } -> std::convertible_to<std::string>;
    };
