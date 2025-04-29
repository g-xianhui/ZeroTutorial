#pragma once

#include <memory>
#include <vector>
#include <unordered_set>

struct TriggerNotify {
    bool is_add;
    std::vector<int> entities;
};

struct AOIEntity {
    int eid;
    float x, y;
    float radius;

    // 我关注谁
    std::unordered_set<int> interests;
    // 谁关注我
    std::unordered_set<int> observers;
    // trigger
    std::vector<TriggerNotify> notifies;
};

struct AOIState {
    int eid;
    // 我看到谁
    std::vector<int> interests;
    // trigger
    std::vector<TriggerNotify> notifies;

    AOIState(AOIEntity& entity) : eid(entity.eid), interests(entity.interests.begin(), entity.interests.end()) {
        notifies.swap(entity.notifies);
    }
};

class AOI : public std::enable_shared_from_this<AOI> {
public:
    virtual ~AOI() {}

    virtual void start() {}
    virtual void stop() {}

    virtual int add_entity(int eid, float x, float y, float radius) = 0;
    virtual void del_entity(int eid) = 0;
    virtual void update_entity(int eid, float x, float y) = 0;

    virtual std::vector<AOIState> fetch_state() = 0;

    // 获取在以(center_x, center_y)为圆心，radius为半径的圆内的所有对象
    virtual std::vector<int> get_entities_in_circle(float center_x, float center_y, float radius) = 0;
    // 获取eid所关心的所有对象
    virtual std::vector<int> get_interests(int eid) = 0;
    // 获取关心eid的所有对象
    virtual std::vector<int> get_observers(int eid) = 0;
};