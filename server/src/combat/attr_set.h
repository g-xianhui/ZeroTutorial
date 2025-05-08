#pragma once

#include "property.h"

class OutputBitStream;

struct AttrSet {
    // 基础属性
    int attack;
    int defence;

    // 特殊属性
    int shield = 0;
    float attack_speed = 1.f;
    float additional_attack_speed = 0.f;
    float critical_rate = 0.f;
    float critical_damage = 0.5f;
    float accuracy = 1.f;
    float dodge_rate = 0.f;
    float tenacity = 0.f;
    float lifesteal = 0.f;

    // 状态（用枚举更规范）
    int status = 0;

    // 初始化方法
    void init() {
        // TODO 配置
        _max_health = 100;
        _max_mana = 100;
        _health = _max_health;
        _mana = _max_mana;

        attack = 10;
        defence = 5;
        clamp_all_attributes();
    }

    void net_serialize(OutputBitStream& bs) const;
    bool net_delta_serialize(OutputBitStream& bs);

    // 受伤害逻辑
    int take_damage(AttrSet& attacker, int attack_damage);

    // 确保属性在合理范围内
    void clamp_all_attributes();

    void add_health(int val);
    void add_mana(int val);

public:
    enum class DirtyFlag {
        max_health = 1 << 0,
        health = 1 << 1,
        max_mana = 1 << 2,
        mana = 1 << 3,
    };

private:
    uint32_t _dirty_flag = 0;

    INT_PROPERTY(max_health);
    INT_PROPERTY(health);
    INT_PROPERTY(max_mana);
    INT_PROPERTY(mana);
};
