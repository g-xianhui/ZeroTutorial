#pragma once

#include <map>
#include <string>
#include <vector>

class Player;

class Space {
public:
    Space(size_t w, size_t h);
    ~Space();

    void join(Player* player);
    void leave(Player* player);
    bool has_player(Player* player);

    void update();

    void call_all(const std::string& msg_name, const std::string& msg_bytes);
    void call_others(Player* player, const std::string& msg_name, const std::string& msg_bytes);

    std::vector<Player*> find_players_in_sector(float cx, float cy, float ux, float uy, float r, float theta);
    std::vector<Player*> find_players_in_circle(float cx, float cy, float r);

private:
    size_t _width;
    size_t _height;

    int _update_timer;

    std::vector<Player*> _players;
};
