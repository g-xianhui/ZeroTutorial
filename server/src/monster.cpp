#include "monster.h"
#include "combat/combat_component.h"
#include "movement_component.h"
#include "space_component.h"
#include "navigation_component.h"

Monster::Monster(int character_id) : Entity(), _character_id(character_id)
{
    add_component<MovementComponent>();
    add_component<SpaceComponent>();
    add_component<CombatComponent>();
    add_component<NavigationComponent>();
    // add_component<AIComponent>();
}

void Monster::net_serialize(OutputBitStream& bs, bool to_self) const
{
    bs.write(_character_id);
}

bool Monster::net_delta_serialize(OutputBitStream& bs, bool to_self)
{
    return false;
}
