#include "space.h"
#include "player.h"

#include "network/tcp_connection.h"

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
    float y = 100.f;
    float z = get_random() * _height;

    player->set_position(x, y, z);

    std::ostringstream buffer;
    // 告知玩家加入场景成功，并附带初始坐标
    buffer << "join_successed#" << player->get_name() << ":" << x << ":" << y << ":" << z;
    std::string join_successed_reply = buffer.str();
    player->send_msg(join_successed_reply.c_str(), join_successed_reply.size());

    buffer.clear();
    buffer << "players_enter_sight#" << player->get_name() << ":" << x << ":" << y << ":" << z;
    std::string enter_other_sight = buffer.str();

    std::string players_enter_sight{"players_enter_sight#"};
    for (Player* other : _players) {
        // 告知场景内的其他玩家，有新玩家进入了场景
        other->send_msg(enter_other_sight.c_str(), enter_other_sight.size());

        float px, py, pz;
        other->get_position(px, py, pz);

        std::ostringstream buffer;
        buffer << other->get_name() << ":" << px << ":" << py << ":" << pz;
        players_enter_sight += buffer.str();
        players_enter_sight += "|";
    }
    // 把场景内所有已存在的玩家信息发送给新玩家
    player->send_msg(players_enter_sight.c_str(), players_enter_sight.size());

    _players.push_back(player);
}

void Space::leave(Player* player)
{
    auto iter = std::find(_players.begin(), _players.end(), player);
    if (iter != _players.end()) {
        Player* player = *iter;

        float x, y, z;
        player->get_position(x, y, z);

        std::ostringstream buffer;
        buffer << "players_leave_sight#" << player->get_name();
        std::string leave_other_sight = buffer.str();

        // 告知其他玩家该player退出了场景
        for (Player* other : _players) {
            if (other == player)
                continue;
            other->send_msg(leave_other_sight.c_str(), leave_other_sight.size());
        }

        _players.erase(iter);
    }
}

bool Space::has_player(Player* player)
{
    auto iter = std::find(_players.begin(), _players.end(), player);
    return iter != _players.end();
}
