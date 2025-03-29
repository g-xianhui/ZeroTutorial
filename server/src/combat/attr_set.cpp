#include "attr_set.h"
#include "math_utils.h"

#include <algorithm>
#include <spdlog/spdlog.h>

int AttrSet::take_damage(AttrSet& attacker, int attack_damage) {
    // 1. 基础伤害计算（至少为1）
    int base_damage = (std::max)(1, attack_damage - this->defence);

    // 2. 暴击判断
    bool is_critical = random_01() < attacker.critical_rate;
    int damage = is_critical ?
        static_cast<int>(base_damage * (1.0f + attacker.critical_damage)) : base_damage;

    // 3. 命中与闪避判断
    float hit_chance = attacker.accuracy - this->dodge_rate;
    bool is_hit = random_01() < hit_chance;
    if (!is_hit) {
        damage = 0;
    }

    // 4. 护盾吸收
    int shield_absorb = (std::min)(damage, this->shield);
    this->shield -= shield_absorb;
    damage -= shield_absorb;

    // 5. 实际扣血（确保不小于0）
    this->health -= damage;
    this->health = (std::max)(0, this->health);

    // 6. 吸血效果（不超过最大生命值）
    if (damage > 0 && attacker.lifesteal > 0) {
        int heal_amount = static_cast<int>(damage * attacker.lifesteal);
        attacker.health += heal_amount;
        attacker.health = (std::min)(attacker.max_health, attacker.health);
    }

    return damage;
}

void AttrSet::clamp_all_attributes() {
    accuracy = std::clamp(accuracy, 0.0f, 1.0f);
    dodge_rate = std::clamp(dodge_rate, 0.0f, 1.0f);
    tenacity = std::clamp(tenacity, 0.0f, 1.0f);
}