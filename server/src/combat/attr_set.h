#pragma once

struct AttrSet {
    // 基础属性
    int max_health;
    int health;
    int max_mana;
    int mana;
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
        health = max_health;
        mana = max_mana;
        clamp_all_attributes();
    }

    // 受伤害逻辑
    int take_damage(AttrSet& attacker, int attack_damage);

    // 确保属性在合理范围内
    void clamp_all_attributes();
};