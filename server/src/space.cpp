#include "space.h"
#include "entity.h"
#include "wheel_timer.h"
#include "math_utils.h"
#include "bit_stream.h"

#include "combat/combat_component.h"
#include "network/tcp_connection.h"
#include "proto/space_service.pb.h"
#include "aoi/aoi_factory.h"

#include "movement_component.h"
#include "space_component.h"
#include "connection_component.h"
#include "navigation_component.h"

#include "monster.h"

#include <spdlog/spdlog.h>

#include <random>
#include <sstream>

const float default_view_raidus = 10.f;

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

struct NavMeshSetHeader
{
    int magic;
    int version;
    int numTiles;
    dtNavMeshParams params;
};

struct NavMeshTileHeader
{
    dtTileRef tileRef;
    int dataSize;
};

dtNavMesh* loadAll(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;

    // Read header.
    NavMeshSetHeader header;
    size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
    if (readLen != 1)
    {
        fclose(fp);
        return 0;
    }
    if (header.magic != NAVMESHSET_MAGIC)
    {
        fclose(fp);
        return 0;
    }
    if (header.version != NAVMESHSET_VERSION)
    {
        fclose(fp);
        return 0;
    }

    dtNavMesh* mesh = dtAllocNavMesh();
    if (!mesh)
    {
        fclose(fp);
        return 0;
    }
    dtStatus status = mesh->init(&header.params);
    if (dtStatusFailed(status))
    {
        fclose(fp);
        return 0;
    }

    // Read tiles.
    for (int i = 0; i < header.numTiles; ++i)
    {
        NavMeshTileHeader tileHeader;
        readLen = fread(&tileHeader, sizeof(tileHeader), 1, fp);
        if (readLen != 1)
        {
            fclose(fp);
            return 0;
        }

        if (!tileHeader.tileRef || !tileHeader.dataSize)
            break;

        unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
        if (!data) break;
        memset(data, 0, tileHeader.dataSize);
        readLen = fread(data, tileHeader.dataSize, 1, fp);
        if (readLen != 1)
        {
            dtFree(data);
            fclose(fp);
            return 0;
        }

        mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
    }

    fclose(fp);

    return mesh;
}

Space::Space(size_t w, size_t h) : _width(w), _height(h)
{
    _update_timer = G_Timer.add_timer(100, [this]() {
        this->update();
    }, true);

    _aoi = create_aoi(AOIType::GRID, 30);
    _aoi->start();

    _nav_mesh = dtAllocNavMesh();
    _nav_mesh = loadAll("DemoScene.bin");
    assert(_nav_mesh);

    _nav_query = dtAllocNavMeshQuery();
    dtStatus status = _nav_query->init(_nav_mesh, 2048);
    if (dtStatusFailed(status)) {
        spdlog::error("nav query  init failed\n");
        assert(0);
    }

    _nav_filter.setIncludeFlags(0xFFFF);
    _nav_filter.setExcludeFlags(0);

    // monster test
    _m = new Monster(1);
    join(_m);
    MovementComponent* movement_comp = _m->get_component<MovementComponent>();
    movement_comp->set_position(4.37f, 0.45f, 14.026f);
}

Space::~Space()
{
    for (auto& [eid, entity] : _eid_2_entity) {
        ConnectionComponent* connection_comp = entity->get_component<ConnectionComponent>();
        if (connection_comp)
            connection_comp->send_msg("kick_out", strlen("kick_out"));
    }

    G_Timer.del_timer(_update_timer);

    _aoi->stop();
}

void Space::join(Entity* entity)
{
    if (has_entity(entity))
        return;

    SpaceComponent* space_comp = entity->get_component<SpaceComponent>();
    space_comp->enter_space(this);

    // 随机一个出生点
    float x = random_01() * _width;
    float y = 3.f;
    float z = random_01() * _height;

    MovementComponent* movement_comp = entity->get_component<MovementComponent>();
    movement_comp->set_position(x, y, z);

    int eid = entity->get_eid();
    _eid_2_entity.insert(std::make_pair(eid, entity));

    _aoi->add_entity(eid, x, z, default_view_raidus);
}

void Space::leave(Entity* entity)
{
    auto iter = _eid_2_entity.find(entity->get_eid());
    if (iter != _eid_2_entity.end()) {
        _aoi->del_entity(entity->get_eid());

        SpaceComponent* space_comp = entity->get_component<SpaceComponent>();
        space_comp->leave_space();
        _eid_2_entity.erase(iter);
    }
}

bool Space::has_entity(Entity* entity)
{
    return _eid_2_entity.contains(entity->get_eid());
}

Entity* Space::find_entity(int eid)
{
    auto iter = _eid_2_entity.find(eid);
    return (iter == _eid_2_entity.end()) ? nullptr : iter->second;
}

void Space::update_position(int eid, float x, float y, float z)
{
    _aoi->update_entity(eid, x, z);
}

void Space::update()
{
    std::unordered_map<int, std::string> entity_properties;
    std::unordered_map<int, std::string> entity_dirty_properties;
    for (auto& iter : _eid_2_entity) {
        int eid = iter.first;
        Entity* entity = iter.second;

        OutputBitStream bs;
        if (entity->entity_net_delta_serialize(bs, true)) {
            // 同步属性变化给自己
            if (entity->has_component<ConnectionComponent>()) {
                space_service::PlayerDeltaInfo delta_info;
                delta_info.set_eid(eid);
                delta_info.set_data(std::string{ bs.get_buffer(), bs.tellp() });

                ConnectionComponent* connection_comp = entity->get_component<ConnectionComponent>();
                send_proto_msg(connection_comp->get_conn(), "sync_delta_info", delta_info);
            }
        }

        // 准备同步给其他人的数据
        bs.seekp(0);
        if (entity->entity_net_delta_serialize(bs, false)) {
            entity_dirty_properties.insert(std::make_pair(eid, std::string{ bs.get_buffer(), bs.tellp() }));
        }

        // 重置状态
        entity->reset_dirty();
    }

    std::vector<AOIState> aoi_state = _aoi->fetch_state();
    for (auto& state : aoi_state) {
        int eid = state.eid;

        Entity* entity = find_entity(eid);
        if (!entity)
            continue;
        
        ConnectionComponent* connection_comp = entity->get_component<ConnectionComponent>();

        // 同步新进入或退出自己视野的玩家给entity
        for (const TriggerNotify& notify : state.notifies) {
            if (notify.is_add) {
                // TODO monster needs this
                // entity->on_others_enter_sight(notify.entities);

                if (connection_comp)
                {
                    space_service::EntitiesEnterSight entity_sight;
                    for (int other_eid : notify.entities) {
                        assert(other_eid != eid);
                        Entity* other = find_entity(other_eid);
                        if (!other)
                            continue;

                        space_service::AoiEntity* aoi_entity = entity_sight.add_entities();
                        aoi_entity->set_entity_type(other->get_type());

                        if (entity_properties.contains(other_eid)) {
                            aoi_entity->set_data(entity_properties[other_eid]);
                        }
                        else {
                            OutputBitStream bs;
                            other->entity_net_serialize(bs, false);
                            auto iter = entity_properties.insert(std::make_pair(other_eid, std::string{ bs.get_buffer(), bs.tellp() }));
                            aoi_entity->set_data(iter.first->second);
                        }

                        space_service::Vector3f* position = aoi_entity->mutable_position();
                        space_service::Vector3f* rotation = aoi_entity->mutable_rotation();
                        MovementComponent* movement_comp = other->get_component<MovementComponent>();
                        float x, y, z;
                        movement_comp->get_position(x, y, z);
                        position->set_x(x);
                        position->set_y(y);
                        position->set_z(z);
                        Rotation r = movement_comp->get_rotation();
                        rotation->set_x(r.pitch);
                        rotation->set_y(r.yaw);
                        rotation->set_z(r.roll);
                    }
                    send_proto_msg(connection_comp->get_conn(), "entities_enter_sight", entity_sight);
                }

            }
            else {
                // TODO monster needs this
                // entity->on_others_leave_sight(notify.entities);

                if (connection_comp) {
                    space_service::EntitiesLeaveSight entity_sight;
                    for (int other_eid : notify.entities) {
                        entity_sight.add_entities(other_eid);
                    }
                    send_proto_msg(connection_comp->get_conn(), "entities_leave_sight", entity_sight);
                }
            }
        }

        // 同步视野内玩家的变化给entity
        if (connection_comp) {
            space_service::AoiUpdates aoi_updates;
            for (int interest_eid : state.interests) {
                Entity* p = find_entity(interest_eid);
                if (p) {
                    auto dirty_property_iter = entity_dirty_properties.find(interest_eid);
                    if (dirty_property_iter != entity_dirty_properties.end()) {
                        space_service::AoiUpdate* aoi_update = aoi_updates.add_datas();
                        aoi_update->set_eid(interest_eid);
                        aoi_update->set_data(dirty_property_iter->second);
                    }
                }
            }
            send_proto_msg(connection_comp->get_conn(), "sync_aoi_update", aoi_updates);
        }
    }
}

void Space::call_all(int eid, const std::string& msg_name, const std::string& msg_bytes)
{
    Entity* entity = find_entity(eid);
    if (entity) {
        ConnectionComponent* connection_comp = entity->get_component<ConnectionComponent>();
        if (connection_comp) {
            send_raw_msg(connection_comp->get_conn(), msg_name, msg_bytes);
        }
    }

    call_others(eid, msg_name, msg_bytes);
}

void Space::call_others(int eid, const std::string& msg_name, const std::string& msg_bytes)
{
    std::vector<int> observers = _aoi->get_observers(eid);
    for (int id : observers) {
        Entity* entity = find_entity(id);
        if (entity) {
            ConnectionComponent* connection_comp = entity->get_component<ConnectionComponent>();
            if (connection_comp) {
                send_raw_msg(connection_comp->get_conn(), msg_name, msg_bytes);
            }
        }
    }
}

std::vector<Entity*> Space::find_entities_in_circle(float cx, float cy, float r)
{
    std::vector<Entity*> entitys;
    std::vector<int> eids = _aoi->get_entities_in_circle(cx, cy, r);
    for (int eid : eids) {
        auto iter = _eid_2_entity.find(eid);
        if (iter != _eid_2_entity.end()) {
            entitys.push_back(iter->second);
        }
    }
    return entitys;
}

std::vector<Vector3f> Space::find_path(const Vector3f& start_pos, const Vector3f& end_pos)
{
    // 设置起点和终点
    float startPos[3] = { -start_pos.x, start_pos.y, start_pos.z };
    float endPos[3] = { -end_pos.x, end_pos.y, end_pos.z };

    float halfExtents[3] = { 2.0f, 4.0f, 2.0f };

    // 查找最近的可行走点
    dtPolyRef startRef, endRef;
    _nav_query->findNearestPoly(startPos, halfExtents, &_nav_filter, &startRef, 0);
    _nav_query->findNearestPoly(endPos, halfExtents, &_nav_filter, &endRef, 0);

    // 计算路径
    dtPolyRef pathPolys[512];  // 存储路径多边形
    int pathCount = 0;
    _nav_query->findPath(startRef, endRef, startPos, endPos, &_nav_filter, pathPolys, &pathCount, 512);

    float epos[3] = { endPos[0], endPos[1], endPos[2] };
    if (pathPolys[pathCount - 1] != endRef)
        _nav_query->closestPointOnPoly(pathPolys[pathCount - 1], endPos, epos, 0);

    // 平滑路径（可选）
    float smoothPath[512 * 3];
    int smoothPathCount = 0;
    _nav_query->findStraightPath(startPos, epos, pathPolys, pathCount, smoothPath, 0, 0, &smoothPathCount, 512);

    std::vector<Vector3f> result;
    for (int i = 0; i < smoothPathCount; i++) {
        result.emplace_back(-smoothPath[i * 3], smoothPath[i * 3 + 1], smoothPath[i * 3 + 2]);
    }

    return result;
}

std::vector<Vector3f> Space::navigation_test(const Vector3f& start_pos, const Vector3f& end_pos)
{
    NavigationComponent* nav_comp = _m->get_component<NavigationComponent>();
    nav_comp->move_to(end_pos);
    return nav_comp->get_path();
}

std::vector<Entity*> Space::find_entities_in_sector(float cx, float cy, float ux, float uy, float r, float theta)
{
    std::vector<Entity*> entitys;
    std::vector<Entity*> circle_entitys = find_entities_in_circle(cx, cy, r);
    for (Entity* entity : circle_entitys) {
        MovementComponent* movement_comp = entity->get_component<MovementComponent>();
        Vector3f pos = movement_comp->get_position();
        if (is_point_in_sector(cx, cy, ux, uy, r, theta, pos.x, pos.z))
            entitys.push_back(entity);
    }
    return entitys;
}
