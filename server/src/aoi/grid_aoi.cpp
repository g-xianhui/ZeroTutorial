#include "grid_aoi.h"

#include <chrono>

GridAOI::GridAOI(size_t grid_size) {
    _grid_size = grid_size;
    _max_view_radius = grid_size * 3 * 0.5f;
}

GridAOI::~GridAOI()
{
}

void GridAOI::start()
{
    _aoi_thread = std::thread([ptr = shared_from_this()]() {
        std::shared_ptr<GridAOI> grid_ptr = std::static_pointer_cast<GridAOI>(ptr);
        grid_ptr->run();
    });
}

void GridAOI::stop()
{
    {
        std::unique_lock<std::mutex> lock(_stop_flag_mutex);
        _is_stop = true;
    }
    _cv.notify_all();

    _aoi_thread.join();
}

void GridAOI::run()
{
    std::unique_lock<std::mutex> lock(_stop_flag_mutex);
    while (!_is_stop) {
        std::unique_lock<std::mutex> data_lock(_data_mutex);
        std::unordered_map<GridPos, Grid> grids{ _grids };
        std::unordered_map<int, AOIEntity> entities{ _entities };
        data_lock.unlock();

        update(grids, entities);

        data_lock.lock();
        _grids.swap(grids);
        _entities.swap(entities);
        data_lock.unlock();

        _cv.wait_for(lock, std::chrono::milliseconds(50));
    }
}

void GridAOI::update(std::unordered_map<GridPos, Grid>& grids, std::unordered_map<int, AOIEntity>& entities)
{
    for (auto& iter : entities) {
        int eid = iter.first;
        AOIEntity& entity = iter.second;

        // 更新interests
        std::vector<int> interests = _get_entities_in_circle(entity.x, entity.y, entity.radius, grids);
        std::unordered_set<int> new_interests{ interests.begin(), interests.end() };
        new_interests.erase(eid);

        // 添加的interests
        std::vector<int> interests_add = calc_set_add(entity.interests, new_interests);
        // 删除的interests
        std::vector<int> interests_del = calc_set_del(entity.interests, new_interests);

        // trigger notify
        if (interests_add.size() > 0) {
            entity.notifies.emplace_back(true, interests_add);
        }
        if (interests_del.size() > 0) {
            entity.notifies.emplace_back(false, interests_del);
        }

        // 对所有添加的interests，将eid注册到它们的observers中
        for (int other_eid : interests_add) {
            AOIEntity& other_entity = _entities[other_eid];
            other_entity.observers.insert(eid);
        }

        // 对所有删除的interests，将eid从它们的observers中删除
        for (int other_eid : interests_del) {
            if (_entities.contains(other_eid)) {
                AOIEntity& other_entity = _entities[other_eid];
                other_entity.observers.erase(eid);
            }
        }

        entity.interests.swap(new_interests);
    }
}

int GridAOI::add_entity(int eid, float x, float y, float radius)
{
    std::lock_guard<std::mutex> lock(_data_mutex);

    auto iter = _entities.find(eid);
    if (iter != _entities.end())
        return -1;

    if (radius > _max_view_radius) {
        radius = _max_view_radius;
    }

    _entities.insert(std::make_pair(eid, AOIEntity{ eid, x, y, radius }));

    GridPos new_grid_pos = calc_grid_pos(x, y);
    auto grid_iter = _grids.find(new_grid_pos);
    if (grid_iter == _grids.end()) {
        grid_iter = _grids.insert(std::make_pair(new_grid_pos, Grid{})).first;
    }
    grid_iter->second.entities.insert(eid);

    return 0;
}

void GridAOI::del_entity(int eid)
{
    std::lock_guard<std::mutex> lock(_data_mutex);

    auto iter = _entities.find(eid);
    if (iter != _entities.end()) {
        AOIEntity& entity = iter->second;
        GridPos grid_pos = calc_grid_pos(entity.x, entity.y);
        auto grid_iter = _grids.find(grid_pos);
        if (grid_iter != _grids.end()) {
            grid_iter->second.entities.erase(eid);
        }

        _entities.erase(iter);
    }
}

void GridAOI::update_entity(int eid, float x, float y)
{
    std::lock_guard<std::mutex> lock(_data_mutex);

    auto iter = _entities.find(eid);
    if (iter == _entities.end())
        return;

    AOIEntity& entity = iter->second;
    GridPos old_grid_pos = calc_grid_pos(entity.x, entity.y);
    GridPos new_grid_pos = calc_grid_pos(x, y);

    // 跨格子移动
    if (old_grid_pos != new_grid_pos) {
        std::vector<GridPos> new_grids = calc_grid_range(x, y);
        std::unordered_set<GridPos> new_grids_set{ new_grids.begin(), new_grids.end() };

        std::vector<GridPos> old_grids = calc_grid_range(entity.x, entity.y);
        std::unordered_set<GridPos> old_grids_set{ old_grids.begin(), old_grids.end() };

        std::vector<GridPos> grids_add = calc_set_add(old_grids_set, new_grids_set);
        std::vector<GridPos> grids_del = calc_set_del(old_grids_set, new_grids_set);

        // 将eid添加到新进入的格子中
        for (GridPos& grid_pos : grids_add) {
            auto grid_iter = _grids.find(grid_pos);
            if (grid_iter == _grids.end()) {
                grid_iter = _grids.insert(std::make_pair(grid_pos, Grid{})).first;
            }
            grid_iter->second.entities.insert(eid);
        }

        // 将eid从离开的旧格子中删除
        for (GridPos& grid_pos : grids_del) {
            auto grid_iter = _grids.find(grid_pos);
            if (grid_iter != _grids.end()) {
                grid_iter->second.entities.erase(eid);
            }
        }
    }

    entity.x = x;
    entity.y = y;
}

std::vector<AOIState> GridAOI::fetch_state()
{
    std::vector<AOIState> result;
    std::lock_guard<std::mutex> lock(_data_mutex);
    for (auto& iter : _entities) {
        result.emplace_back(iter.second);
    }
    return result;
}

std::vector<int> GridAOI::_get_entities_in_circle(float x, float y, float radius, const std::unordered_map<GridPos, Grid>& grids)
{
    std::vector<int> result;

    int min_grid_x = static_cast<int>((x - radius) / _grid_size);
    int max_grid_x = static_cast<int>((x + radius) / _grid_size);
    int min_grid_y = static_cast<int>((y - radius) / _grid_size);
    int max_grid_y = static_cast<int>((y + radius) / _grid_size);

    for (int grid_x = min_grid_x; grid_x <= max_grid_x; grid_x++) {
        for (int grid_y = min_grid_y; grid_y <= max_grid_y; grid_y++) {
            GridPos pos{ grid_x, grid_y };
            auto grid_iter = grids.find(pos);
            if (grid_iter != grids.end()) {
                const Grid& grid = grid_iter->second;
                for (int eid : grid.entities) {
                    AOIEntity& entity = _entities[eid];
                    float dist = std::hypot(x - entity.x, y - entity.y);
                    if (dist < radius) {
                        result.push_back(eid);
                    }
                }
            }
        }
    }

    return result;
}

std::vector<int> GridAOI::get_entities_in_circle(float x, float y, float radius)
{
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _get_entities_in_circle(x, y, radius, _grids);
}

std::vector<int> GridAOI::get_interests(int eid)
{
    std::lock_guard<std::mutex> lock(_data_mutex);

    auto iter = _entities.find(eid);
    if (iter == _entities.end())
        return std::vector<int>();

    AOIEntity& entity = iter->second;
    return std::vector<int>{entity.interests.begin(), entity.interests.end()};
}

std::vector<int> GridAOI::get_observers(int eid)
{
    std::lock_guard<std::mutex> lock(_data_mutex);

    auto iter = _entities.find(eid);
    if (iter == _entities.end())
        return std::vector<int>();

    AOIEntity& entity = iter->second;
    return std::vector<int>{entity.observers.begin(), entity.observers.end()};
}

GridPos GridAOI::calc_grid_pos(float x, float y) const
{
    int grid_x = static_cast<int>(x / _grid_size);
    int grid_y = static_cast<int>(y / _grid_size);
    return GridPos{ grid_x, grid_y };
}

std::vector<GridPos> GridAOI::calc_grid_range(float x, float y) const
{
    std::vector<GridPos> result;

    GridPos grid_pos = calc_grid_pos(x, y);
    for (int dx = -1; dx < 2; dx++) {
        for (int dy = -1; dy < 2; dy++) {
            GridPos pos{ grid_pos.x + dx, grid_pos.y + dy };
            result.push_back(pos);
        }
    }

    return result;
}
