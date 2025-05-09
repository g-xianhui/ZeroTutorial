#pragma once

#include <cassert>
#include <string>
#include <unordered_map>

#include "icomponent.h"

class Space;

class Entity {
public:
    Entity();
    virtual ~Entity();

    virtual void net_serialize(OutputBitStream& bs, bool to_self) const {}
    virtual bool net_delta_serialize(OutputBitStream& bs, bool to_self) { return false; }
    virtual void reset_dirty() { _dirty_flag = 0; }

    virtual std::string get_type() { return std::string{ "Entity" }; }

    void entity_net_serialize(OutputBitStream& bs, bool to_self) const;
    bool entity_net_delta_serialize(OutputBitStream& bs, bool to_self);
    void entity_reset_dirty();

    inline int get_eid() const { return _eid; }
    inline void set_eid(int eid) { _eid = eid; }

    template< IsComponent T, class... Args >
    void add_component(Args&&... args);

    template<IsComponent T>
    void remove_component();

    template<IsComponent T>
    T* get_component();

    template<IsComponent T>
    bool has_component();

private:
    int _eid;
    std::unordered_map<std::string, IComponent*> _components;

protected:
    uint32_t _dirty_flag = 0;
};

template<IsComponent T, class ...Args>
inline void Entity::add_component(Args && ...args)
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    assert(iter == _components.end());

    IComponent* comp = new T(this, std::forward<Args>(args)...);
    comp->init();
    _components[name] = comp;
}

template<IsComponent T>
void Entity::remove_component()
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    if (iter != _components.end()) {
        IComponent* comp = iter->second;
        comp->destroy();
        delete comp;

        _components.erase(iter);
    }
}

template<IsComponent T>
T* Entity::get_component()
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    return iter == _components.end() ? nullptr : static_cast<T*>(iter->second);
}

template<IsComponent T>
inline bool Entity::has_component()
{
    const std::string& name = T::COMPONENT_NAME;
    auto iter = _components.find(name);
    return iter == _components.end() ? false : true;
}
