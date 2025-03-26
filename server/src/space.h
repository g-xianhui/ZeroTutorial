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

    void normal_attack(Player* player, int combo_seq);

    void call_all(const std::string& msg_name, const std::string& msg_bytes);

private:
    size_t _width;
    size_t _height;

    int _update_timer;

    std::vector<Player*> _players;
};
