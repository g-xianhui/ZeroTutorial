#pragma once

#include <string>

#include "entity.h"
#include "icomponent.h"
#include "property.h"

class TcpConnection;
class OutputBitStream;

class Player : public Entity {
public:
    // conn的生命周期由外部管理
    Player(TcpConnection* conn, const std::string& name);
    ~Player();

    void net_serialize(OutputBitStream& bs, bool to_self) const override;
    bool net_delta_serialize(OutputBitStream& bs, bool to_self) override;

    std::string get_type() override { return std::string{ "Player" }; }

    void play_animation(const std::string& name, float speed = 1.f, bool sync_to_all = false);

public:
    enum class DirtyFlag {
        name = 1,
    };

private:
    STR_PROPERTY(name);
};
