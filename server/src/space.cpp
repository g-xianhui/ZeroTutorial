#include "space.h"
#include "player.h"
#include "wheel_timer.h"
#include "math_utils.h"

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

void get_movement_data(Player* p, space_service::Movement* new_move)
{
    Vector3f cur_position = p->get_position();
    Rotation cur_rotation = p->get_rotation();
    Vector3f cur_velocity = p->get_velocity();
    Vector3f cur_acceleration = p->get_acceleration();
    Vector3f cur_angular_velocity = p->get_angular_velocity();

    space_service::Vector3f* position = new_move->mutable_position();
    position->set_x(cur_position.x);
    position->set_y(cur_position.y);
    position->set_z(cur_position.z);

    space_service::Vector3f* rotation = new_move->mutable_rotation();
    rotation->set_x(cur_rotation.pitch);
    rotation->set_y(cur_rotation.yaw);
    rotation->set_z(cur_rotation.roll);

    space_service::Vector3f* velocity = new_move->mutable_velocity();
    velocity->set_x(cur_velocity.x);
    velocity->set_y(cur_velocity.y);
    velocity->set_z(cur_velocity.z);

    space_service::Vector3f* acceleration = new_move->mutable_acceleration();
    acceleration->set_x(cur_acceleration.x);
    acceleration->set_y(cur_acceleration.y);
    acceleration->set_z(cur_acceleration.z);

    space_service::Vector3f* angular_velocity = new_move->mutable_angular_velocity();
    angular_velocity->set_x(cur_angular_velocity.x);
    angular_velocity->set_y(cur_angular_velocity.y);
    angular_velocity->set_z(cur_angular_velocity.z);

    new_move->set_mode(p->get_move_mode());
    new_move->set_timestamp(p->get_move_timestamp());
}


Space::Space(size_t w, size_t h) : _width(w), _height(h)
{
    _update_timer = G_Timer.add_timer(100, [this]() {
        this->update();
    }, true);
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

    player->enter_space(this);

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
    space_service::Movement* new_transform = aoi_player->mutable_transform();
    get_movement_data(player, new_transform);

    space_service::PlayersEnterSight player_sight;
    for (Player* other : _players) {
        if (!other->get_conn())
            continue;

        // 告知场景内的其他玩家，有新玩家进入了场景
        send_proto_msg(other->get_conn(), "players_enter_sight", others_sight);

        space_service::AoiPlayer* aoi_player = player_sight.add_players();
        aoi_player->set_name(other->get_name());
        space_service::Movement* new_transform = aoi_player->mutable_transform();
        get_movement_data(other, new_transform);
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

        player->leave_space();
        _players.erase(iter);
    }
}

bool Space::has_player(Player* player)
{
    auto iter = std::find(_players.begin(), _players.end(), player);
    return iter != _players.end();
}

void Space::update()
{
    space_service::PlayerMovements player_movements;
    for (Player* p : _players) {
        space_service::PlayerMovement* data = player_movements.add_datas();
        data->set_name(p->get_name());

        space_service::Movement* new_move = data->mutable_data();
        get_movement_data(p, new_move);
    }

    for (Player* p : _players) {
        send_proto_msg(p->get_conn(), "sync_movement", player_movements);
    }
}

void Space::call_all(const std::string& msg_name, const std::string& msg_bytes)
{
    for (Player* player : _players) {
        send_raw_msg(player->get_conn(), msg_name, msg_bytes);
    }
}

void Space::call_others(Player* player, const std::string& msg_name, const std::string& msg_bytes)
{
    for (Player* other : _players) {
        if (other == player)
            continue;
        send_raw_msg(other->get_conn(), msg_name, msg_bytes);
    }
}

std::vector<Player*> Space::find_players_in_circle(float cx, float cy, float r)
{
    std::vector<Player*> players;
    for (Player* player : _players) {
        Vector3f pos = player->get_position();
        if (is_point_in_circle(cx, cy, r, pos.x, pos.z))
            players.push_back(player);
    }
    return players;
}

std::vector<Player*> Space::find_players_in_sector(float cx, float cy, float ux, float uy, float r, float theta)
{
    std::vector<Player*> players;
    for (Player* player : _players) {
        Vector3f pos = player->get_position();
        if (is_point_in_sector(cx, cy, ux, uy, r, theta, pos.x, pos.z))
            players.push_back(player);
    }
    return players;
}
