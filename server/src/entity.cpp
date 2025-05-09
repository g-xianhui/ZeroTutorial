#include "entity.h"
#include "bit_stream.h"

static int gen_eid() {
    static int G_IDX = 0;
    G_IDX += 1;
    return G_IDX;
}

Entity::Entity() : _eid(gen_eid())
{
}

Entity::~Entity()
{
    for (auto [_, comp] : _components) {
        comp->destroy();
        delete comp;
    }
    _components.clear();
}

void Entity::entity_net_serialize(OutputBitStream& bs, bool to_self) const
{
    bs.write(_eid);

    net_serialize(bs, to_self);

    for (auto& [name, comp] : _components) {
        bs.write(name);
        comp->net_serialize(bs, to_self);
    }
}

bool Entity::entity_net_delta_serialize(OutputBitStream& bs, bool to_self)
{
    bool dirty = false;

    bs.write(_dirty_flag);
    if (_dirty_flag) {
        dirty = true;
        net_delta_serialize(bs, to_self);
    }

    for (auto& [name, comp] : _components) {
        bs.write(name);
        dirty |= comp->net_delta_serialize(bs, to_self);
    }
    return dirty;
}

void Entity::entity_reset_dirty()
{
    reset_dirty();
    for (auto& [_, comp] : _components) {
        comp->reset_dirty();
    }
}
