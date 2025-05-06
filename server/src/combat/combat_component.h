#pragma once

#include "icomponent.h"
#include "combat/skill/iskill.h"
#include "combat/attr_set.h"

#include "proto/space_service.pb.h"

#include "sync_array.h"
#include "bit_utils.h"

#include <vector>
#include <map>

class SkillInfo
{
public:
    void init(int skill_id, const std::string& anim, int cost_mana, int cool_down, bool local_predicated) {
        _skill_id = skill_id;
        _anmimator_state = anim;
        _cost_mana = cost_mana;
        _cool_down = cool_down;
        _local_predicated = local_predicated;
    }

    bool instance_per_entity = true;

    enum class DirtyFlag : uint8_t {
        skill_id = 1,
        anmimator_state = 2,
        cost_mana = 4,
        cool_down = 8,
        next_cast_time = 16,
        local_predicated = 32,
    };

    INT_GETSET(skill_id)
    STR_GETSET(anmimator_state)
    INT_GETSET(cost_mana)
    INT_GETSET(cool_down)
    INT_GETSET(next_cast_time)
    BOOL_GETSET(local_predicated)

    void net_serialize(OutputBitStream& bs) const {
        bs.write(_skill_id);
        bs.write(_anmimator_state);
        bs.write(_cost_mana);
        bs.write(_cool_down);
        bs.write(_next_cast_time);
        bs.write(_local_predicated);
    }
    
    bool net_delta_serialize(OutputBitStream& bs) {
        bool dirty = false;
        bs.write(_dirty_flag);
        if (_dirty_flag) {
            dirty = true;

            if (_dirty_flag & (uint8_t)DirtyFlag::skill_id) {
                bs.write(_skill_id);
            }

            if (_dirty_flag & (uint8_t)DirtyFlag::anmimator_state) {
                bs.write(_anmimator_state);
            }

            if (_dirty_flag & (uint8_t)DirtyFlag::cost_mana) {
                bs.write(_cost_mana);
            }

            if (_dirty_flag & (uint8_t)DirtyFlag::cool_down) {
                bs.write(_cool_down);
            }

            if (_dirty_flag & (uint8_t)DirtyFlag::next_cast_time) {
                bs.write(_next_cast_time);
            }

            if (_dirty_flag & (uint8_t)DirtyFlag::local_predicated) {
                bs.write(_local_predicated);
            }

            _dirty_flag = 0;
        }
        return dirty;
    }

private:
    int _skill_id;
    std::string _anmimator_state;

    int _cost_mana = 0;
    // ms
    int _cool_down = 0;
    int _next_cast_time = 0;

    bool _local_predicated = false;

    uint8_t _dirty_flag = 0;
};

class CombatComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "CombatComponent";

    virtual void start() override;
    virtual void stop() override;
    virtual void net_serialize(OutputBitStream& bs) const override;
    virtual bool net_delta_serialize(OutputBitStream& bs) override;

    void normal_attack(int combo_seq);
    void cast_skill(int skill_id);
    bool can_cast_skill(const SkillInfo& info);
    
    void take_damage(CombatComponent* attacker, int damage);

private:
    ISkill* get_or_create_skill_instance(int skill_id, bool instance_per_entity = true);
    ISkill* create_skill_instance(int skill_id);
    void stop_running_skill();

private:
    AttrSet _attr_set;

    TSyncArray<SkillInfo> _skill_infos;
    std::map<int, ISkill*> _running_skills;

    size_t _next_normal_attack_time = 0;
    int _normal_attack_timer = -1;
};
