#pragma once

#include <map>
#include <string>
#include <vector>

class Player;

class Space {
public:
    Space(size_t w, size_t h) : _width(w), _height(h) {}
    ~Space();

    void join(Player* player);
    void leave(Player* player);
    bool has_player(Player* player);

private:
    size_t _width;
    size_t _height;

    std::vector<Player*> _players;
};
