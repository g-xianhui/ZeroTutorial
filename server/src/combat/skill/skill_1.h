#pragma once

#include "skill_factory.h"

class Skill_1 : public ISkill {
public:
    virtual void destroy() override;
    virtual void execute() override;

private:
    int _effect_timer = -1;
};
