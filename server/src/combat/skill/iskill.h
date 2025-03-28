#pragma once

class Player;

class ISkill {
public:
    virtual ~ISkill() = default;

    virtual void execute(Player*) = 0;
};