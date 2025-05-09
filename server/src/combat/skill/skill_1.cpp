#include "combat/skill/skill_1.h"
#include "combat/combat_component.h"
#include "space_component.h"
#include "movement_component.h"

#include "player.h"
#include "space.h"
#include "wheel_timer.h"
#include <cassert>

REGISTER_SKILL(Skill_1, 1);

void Skill_1::destroy()
{
    if (_effect_timer != -1)
    {
        G_Timer.del_timer(_effect_timer);
        _effect_timer = -1;
    }
}

// 暂定效果为: 飞行空中，1.5秒后落地，并对范围内敌人造成10点伤害
void Skill_1::execute() {
    CombatComponent* combat_comp = get_owner();
    assert(combat_comp);

    // FIXME 可能是其他entity
    Player* player = static_cast<Player*>(combat_comp->get_owner());
    assert(player);
    player->play_animation("Skill1", 1.f, true);

    _effect_timer = G_Timer.add_timer(1500, [this]() {
        CombatComponent* combat_comp = get_owner();
        assert(combat_comp);

        Entity* player = combat_comp->get_owner();
        assert(player);

        SpaceComponent* space_comp = player->get_component<SpaceComponent>();
        assert(space_comp);
        MovementComponent* movement_comp = player->get_component<MovementComponent>();
        assert(movement_comp);

        Vector3f center = movement_comp->get_position();
        Space* space = space_comp->get_space();

        std::vector<Entity*> others = space->find_entities_in_circle(center.x, center.z, 2.f);
        for (Entity* other : others) {
            if (other == player)
                continue;

            CombatComponent* other_combat_comp = other->get_component<CombatComponent>();
            if (other_combat_comp)
                other_combat_comp->take_damage(combat_comp, 10);
        }

        _effect_timer = -1;

        // 在下一个技能启动，或者player销毁时才会触发本skill的销毁
        set_active(false);
    });
}