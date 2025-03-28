#include "skill_1.h"
#include "player.h"
#include "space.h"
#include "wheel_timer.h"
#include <iostream>

REGISTER_SKILL(Skill_1, 1);

// 暂定效果为: 飞行空中，1.5秒后落地，并对范围内敌人造成10点伤害
void Skill_1::execute(Player* owner) {
    owner->play_animation("Skill1");

    // FIXME 指针安全
    G_Timer.add_timer(1500, [owner]() {
        Vector3f center = owner->get_position();
        Space* space = owner->get_space();
        std::vector<Player*> others = space->find_players_in_circle(center.x, center.z, 2.f);
        for (Player* other : others) {
            if (other == owner)
                continue;
            other->take_damage(owner, 10);
        }
    });
}