#pragma once

#include "skill_factory.h"

class Skill_1 : public ISkill {
public:
    void execute(Player* owner) override;
};
