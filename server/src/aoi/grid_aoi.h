#pragma once

#include "aoi.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

struct GridPos {
    int x, y;
    bool operator==(const GridPos& other) const {
        return x == other.x && y == other.y;
    }
};

namespace std {
    template<>
    struct hash<GridPos> {
        size_t operator()(const GridPos& pos) const {
            return hash<int>()(pos.x) ^ hash<int>()(pos.y);
        }
    };
}

struct Grid {
    std::unordered_set<int> entities;
};

class GridAOI : public AOI {
public:
    GridAOI(size_t grid_size);
    ~GridAOI();

    // Inherited via AOI
    void start() override;

    void stop() override;

    int add_entity(int eid, float x, float y, float radius) override;

    void del_entity(int eid) override;

    void update_entity(int eid, float x, float y) override;

    std::vector<AOIState> fetch_state() override;

    std::vector<int> get_entities_in_circle(float x, float y, float radius) override;

    std::vector<int> get_interests(int eid) override;

    std::vector<int> get_observers(int eid) override;

private:
    GridPos calc_grid_pos(float x, float y) const;
    std::vector<GridPos> calc_grid_range(float x, float y) const;

    void run();

    void update(std::unordered_map<GridPos, Grid>& grids, std::unordered_map<int, AOIEntity>& entities);

    std::vector<int> _get_entities_in_circle(float x, float y, float radius, const std::unordered_map<GridPos, Grid>& grids);

private:
    size_t _grid_size;
    float _max_view_radius;

    std::mutex _data_mutex;

    bool _is_stop = false;
    std::mutex _stop_flag_mutex;
    std::condition_variable _cv;

    std::thread _aoi_thread;
    std::unordered_map<GridPos, Grid> _grids;
    std::unordered_map<int, AOIEntity> _entities;
};

// 找出b相对于a新增的元素
template<typename T>
std::vector<T> calc_set_add(const std::unordered_set<T>& a, const std::unordered_set<T>& b) {
    std::vector<T> result;
    for (T x : b) {
        if (!a.contains(x))
            result.push_back(x);
    }
    return result;
}

// 找出b相对于a减少的元素
template<typename T>
std::vector<T> calc_set_del(const std::unordered_set<T>& a, const std::unordered_set<T>& b) {
    std::vector<T> result;
    for (T x : a) {
        if (!b.contains(x))
            result.push_back(x);
    }
    return result;
}