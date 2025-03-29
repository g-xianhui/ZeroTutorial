#pragma once

#include "icomponent.h"
#include "combat/skill/iskill.h"
#include "combat/attr_set.h"

#include <vector>

class CombatComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "CombatComponent";

    virtual void start() override;
    virtual void stop() override;

    void normal_attack(int combo_seq);
    void cast_skill(int skill_id);
    void stop_running_skill();

    void take_damage(CombatComponent* attacker, int damage);

private:
    AttrSet _attr_set;
    std::vector<int> _skills;

    size_t _next_normal_attack_time = 0;
    int _normal_attack_timer = -1;

    ISkill* _running_skill = nullptr;
};