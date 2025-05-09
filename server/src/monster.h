#pragma once

#include "entity.h"

class Space;

class Monster : Entity {
public:
    Monster();

    void net_serialize(OutputBitStream& bs, bool to_self) const override;
    bool net_delta_serialize(OutputBitStream& bs, bool to_self) override;

    std::string get_type() override { return std::string{ "Monster" }; }

private:
    int _character_id = 0;
};