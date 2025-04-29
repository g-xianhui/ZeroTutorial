#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class Player;
class AOI;

class Space {
public:
    Space(size_t w, size_t h);
    ~Space();

    void join(Player* player);
    void leave(Player* player);
    bool has_player(Player* player);
    Player* find_player(int eid);

    void update_position(int eid, float x, float y, float z);

    void update();

    void call_all(const std::string& msg_name, const std::string& msg_bytes);
    void call_others(Player* player, const std::string& msg_name, const std::string& msg_bytes);

    std::vector<Player*> find_players_in_sector(float cx, float cy, float ux, float uy, float r, float theta);
    std::vector<Player*> find_players_in_circle(float cx, float cy, float r);

private:
    size_t _width;
    size_t _height;

    int _update_timer;

    std::unordered_map<int, Player*> _eid_2_player;
    std::vector<Player*> _players;

    std::shared_ptr<AOI> _aoi;
};
