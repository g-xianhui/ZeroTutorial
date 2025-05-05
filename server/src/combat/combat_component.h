#pragma once

#include "icomponent.h"
#include "combat/skill/iskill.h"
#include "combat/attr_set.h"

#include "proto/space_service.pb.h"

#include <vector>
#include <map>

struct SkillInfo
{
    int skill_id;
    std::string anmimator_state;

    int cost_mana = 0;
    // ms
    int cool_down = 0;
    int next_cast_time = 0;

    bool local_predicated = false;
    bool instance_per_entity = true;
};

class CombatComponent : public IComponent {
public:
    static inline const char* COMPONENT_NAME = "CombatComponent";

    virtual void start() override;
    virtual void stop() override;
    virtual void net_serialize(OutputBitStream& bs) override;
    virtual bool consume_dirty(OutputBitStream& bs) override;

    void normal_attack(int combo_seq);
    void cast_skill(int skill_id);
    bool can_cast_skill(const SkillInfo& info);
    
    void take_damage(CombatComponent* attacker, int damage);

    void fill_proto_attr_set(space_service::AttrSet& msg);
    void sync_attr_set();
    void sync_skill_info(const SkillInfo& skill_info);

private:
    ISkill* get_or_create_skill_instance(int skill_id, bool instance_per_entity = true);
    ISkill* create_skill_instance(int skill_id);
    void stop_running_skill();

private:
    AttrSet _attr_set;

    std::vector<SkillInfo> _skill_infos;
    std::map<int, ISkill*> _running_skills;

    size_t _next_normal_attack_time = 0;
    int _normal_attack_timer = -1;
};
