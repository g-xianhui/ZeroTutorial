#include "space.h"
#include "player.h"

#include "network/tcp_connection.h"

#include "proto/space_service.pb.h"

#include <random>
#include <sstream>

float get_random()
{
    static std::default_random_engine e;
    static std::uniform_real_distribution<float> dis(0.f, 1.f);
    return dis(e);
}

Space::~Space()
{
    for (Player* player : _players) {
        player->send_msg("kick_out", strlen("kick_out"));
    }
}

void Space::join(Player* player)
{
    if (has_player(player))
        return;

    // 随机一个出生点
    float x = get_random() * _width;
    float y = 3.f;
    float z = get_random() * _height;

    player->set_position(x, y, z);

    // 告知玩家加入场景成功，并附带初始坐标
    space_service::JoinReply join_reply;
    join_reply.set_result(0);
    space_service::Vector3f* position = join_reply.mutable_position();
    position->set_x(x);
    position->set_y(y);
    position->set_z(z);
    send_proto_msg(player->get_conn(), "join_reply", join_reply);

    space_service::PlayersEnterSight others_sight;
    space_service::AoiPlayer* aoi_player = others_sight.add_players();
    aoi_player->set_name(player->get_name());
    space_service::Vector3f* aoi_position = aoi_player->mutable_position();
    aoi_position->set_x(x);
    aoi_position->set_y(y);
    aoi_position->set_z(z);

    space_service::PlayersEnterSight player_sight;
    for (Player* other : _players) {
        if (!other->get_conn())
            continue;

        // 告知场景内的其他玩家，有新玩家进入了场景
        send_proto_msg(other->get_conn(), "players_enter_sight", others_sight);

        float px, py, pz;
        other->get_position(px, py, pz);

        space_service::AoiPlayer* aoi_player = player_sight.add_players();
        aoi_player->set_name(other->get_name());
        space_service::Vector3f* aoi_position = aoi_player->mutable_position();
        aoi_position->set_x(px);
        aoi_position->set_y(py);
        aoi_position->set_z(pz);
    }
    // 把场景内所有已存在的玩家信息发送给新玩家
    send_proto_msg(player->get_conn(), "players_enter_sight", player_sight);

    _players.push_back(player);
}

void Space::leave(Player* player)
{
    auto iter = std::find(_players.begin(), _players.end(), player);
    if (iter != _players.end()) {
        Player* player = *iter;
        space_service::PlayersLeaveSight sight;
        std::string* player_name = sight.add_players();
        *player_name = player->get_name();

        // 告知其他玩家该player退出了场景
        for (Player* other : _players) {
            if (other == player)
                continue;
            send_proto_msg(other->get_conn(), "players_leave_sight", sight);
        }

        _players.erase(iter);
    }
}

bool Space::has_player(Player* player)
{
    auto iter = std::find(_players.begin(), _players.end(), player);
    return iter != _players.end();
}
