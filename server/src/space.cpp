#include "space.h"
#include "player.h"
#include "wheel_timer.h"
#include "math_utils.h"
#include "bit_stream.h"

#include "combat/combat_component.h"
#include "network/tcp_connection.h"
#include "proto/space_service.pb.h"
#include "aoi/aoi_factory.h"

#include <random>
#include <sstream>

const float default_view_raidus = 10.f;

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

    _aoi = create_aoi(AOIType::GRID, 30);
    _aoi->start();
}

Space::~Space()
{
    for (auto& [eid, player] : _eid_2_player) {
        player->send_msg("kick_out", strlen("kick_out"));
    }

    G_Timer.del_timer(_update_timer);

    _aoi->stop();
}

void Space::join(Player* player)
{
    if (has_player(player))
        return;

    player->enter_space(this);

    // 随机一个出生点
    float x = random_01() * _width;
    float y = 3.f;
    float z = random_01() * _height;

    player->set_position(x, y, z);

    int eid = player->get_eid();
    _eid_2_player.insert(std::make_pair(eid, player));

    _aoi->add_entity(eid, x, z, default_view_raidus);

    // 告知玩家加入场景成功，并附带初始坐标
    space_service::JoinReply join_reply;
    join_reply.set_result(0);
    space_service::Vector3f* position = join_reply.mutable_position();
    position->set_x(x);
    position->set_y(y);
    position->set_z(z);
    send_proto_msg(player->get_conn(), "join_reply", join_reply);

    // 同步属性给自己
    OutputBitStream bs;
    player->net_serialize(bs);
    space_service::PlayerInfo player_info;
    player_info.set_eid(player->get_eid());
    player_info.set_data(std::string{ bs.get_buffer(), bs.tellp() });
    send_proto_msg(player->get_conn(), "sync_full_info", player_info);
}

void Space::leave(Player* player)
{
    auto iter = _eid_2_player.find(player->get_eid());
    if (iter != _eid_2_player.end()) {
        _aoi->del_entity(player->get_eid());
        player->leave_space();
        _eid_2_player.erase(iter);
    }
}

bool Space::has_player(Player* player)
{
    return _eid_2_player.contains(player->get_eid());
}

Player* Space::find_player(int eid)
{
    auto iter = _eid_2_player.find(eid);
    return (iter == _eid_2_player.end()) ? nullptr : iter->second;
}

void Space::update_position(int eid, float x, float y, float z)
{
    _aoi->update_entity(eid, x, z);
}

void Space::update()
{
    std::unordered_map<int, std::string> entity_properties;
    std::unordered_map<int, std::string> entity_dirty_properties;
    for (auto& iter : _eid_2_player) {
        int eid = iter.first;
        Player* player = iter.second;

        OutputBitStream bs;
        if (player->consume_dirty(bs)) {
            auto result = entity_dirty_properties.insert(std::make_pair(eid, std::string{bs.get_buffer(), bs.tellp()}));

            // 同步属性变化给自己
            space_service::PlayerDeltaInfo delta_info;
            delta_info.set_eid(eid);
            delta_info.set_data(result.first->second);
            send_proto_msg(player->get_conn(), "sync_delta_info", delta_info);
        }
    }

    std::vector<AOIState> aoi_state = _aoi->fetch_state();
    for (auto& state : aoi_state) {
        int eid = state.eid;

        Player* player = find_player(eid);
        if (!player)
            continue;

        // 同步新进入或退出自己视野的玩家给player
        for (const TriggerNotify& notify : state.notifies) {
            if (notify.is_add) {
                space_service::PlayersEnterSight player_sight;
                for (int other_eid : notify.entities) {
                    Player* other = find_player(other_eid);
                    if (!other)
                        continue;

                    space_service::AoiPlayer* aoi_player = player_sight.add_players();
                    aoi_player->set_eid(other->get_eid());
                    if (entity_properties.contains(other_eid)) {
                        aoi_player->set_data(entity_properties[other_eid]);
                    } else {
                        OutputBitStream bs;
                        other->net_serialize(bs);
                        auto iter = entity_properties.insert(std::make_pair(eid, std::string{bs.get_buffer(), bs.tellp()}));
                        aoi_player->set_data(iter.first->second);
                    }

                    space_service::Movement* new_transform = aoi_player->mutable_transform();
                    get_movement_data(other, new_transform);
                }
                send_proto_msg(player->get_conn(), "players_enter_sight", player_sight);
            }
            else {
                space_service::PlayersLeaveSight player_sight;
                for (int other_eid : notify.entities) {
                    player_sight.add_players(other_eid);
                }
                send_proto_msg(player->get_conn(), "players_leave_sight", player_sight);
            }
        }

        // 同步视野内玩家的移动给player
        space_service::PlayerMovements player_movements;
        // 同步视野内玩家的属性变化给player
        space_service::AoiUpdates aoi_updates;
        for (int interest_eid : state.interests) {
            Player* p = find_player(interest_eid);
            if (p) {
                space_service::PlayerMovement* data = player_movements.add_datas();
                data->set_eid(interest_eid);

                space_service::Movement* new_move = data->mutable_data();
                get_movement_data(p, new_move);

                auto dirty_property_iter = entity_dirty_properties.find(interest_eid);
                if (dirty_property_iter != entity_dirty_properties.end()) {
                    space_service::AoiUpdate* aoi_update = aoi_updates.add_datas();
                    aoi_update->set_eid(interest_eid);
                    aoi_update->set_data(dirty_property_iter->second);
                }
            }
        }
        send_proto_msg(player->get_conn(), "sync_movement", player_movements);
        send_proto_msg(player->get_conn(), "sync_aoi_update", aoi_updates);
    }
}

void Space::call_all(int eid, const std::string& msg_name, const std::string& msg_bytes)
{
    Player* player = find_player(eid);
    if (player) {
        send_raw_msg(player->get_conn(), msg_name, msg_bytes);
    }

    call_others(eid, msg_name, msg_bytes);
}

void Space::call_others(int eid, const std::string& msg_name, const std::string& msg_bytes)
{
    std::vector<int> observers = _aoi->get_observers(eid);
    for (int id : observers) {
        Player* player = find_player(id);
        if (player) {
            send_raw_msg(player->get_conn(), msg_name, msg_bytes);
        }
    }
}

std::vector<Player*> Space::find_players_in_circle(float cx, float cy, float r)
{
    std::vector<Player*> players;
    std::vector<int> eids = _aoi->get_entities_in_circle(cx, cy, r);
    for (int eid : eids) {
        auto iter = _eid_2_player.find(eid);
        if (iter != _eid_2_player.end()) {
            players.push_back(iter->second);
        }
    }
    return players;
}

std::vector<Player*> Space::find_players_in_sector(float cx, float cy, float ux, float uy, float r, float theta)
{
    std::vector<Player*> players;
    std::vector<Player*> circle_players = find_players_in_circle(cx, cy, r);
    for (Player* player : circle_players) {
        Vector3f pos = player->get_position();
        if (is_point_in_sector(cx, cy, ux, uy, r, theta, pos.x, pos.z))
            players.push_back(player);
    }
    return players;
}
