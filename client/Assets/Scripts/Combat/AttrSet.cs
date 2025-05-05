using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class AttrSet
{
    public int MaxHealth;
    public int Health;
    public int MaxMana;
    public int Mana;
    public int Attack;
    public float AttackSpeed = 1;
    public float AdditionalAttackSpeed = 0;

    public int Defence;
    public float CriticalRate;
    public float CriticalDamage;
    // 命中率
    [Range(0.0f, 1.0f)]
    public float Accuracy;
    // 闪避
    [Range(0.0f, 1.0f)]
    public float DodgeRate;
    // 护盾
    public int Shield;
    // 韧性
    [Range(0.0f, 1.0f)]
    public float Tenacity;
    // 吸血
    [Range(0.0f, 1.0f)]
    public float Lifesteal;

    // 状态（晕眩、冻结、霸体。。。）
    public int Status;

    public void NetSerialize(BinaryReader br)
    {
        MaxHealth = br.ReadInt32();
        Health = br.ReadInt32();
        MaxMana = br.ReadInt32();
        Mana = br.ReadInt32();
        Debug.Log($"MaxHealth: {MaxHealth}, Mana: {Mana}");
    }

    public void TakeDamage(AttrSet attacker, int attackDamage)
    {
        // 1. 基础伤害计算
        int baseDamage = Mathf.Max(1, attackDamage - this.Defence);

        // 2. 暴击判断
        bool isCritical = Random.value < attacker.CriticalRate;
        float damage = isCritical ? baseDamage * (1 + attacker.CriticalDamage) : baseDamage;

        // 3. 命中与闪避判断
        bool isHit = Random.value < (attacker.Accuracy - this.DodgeRate);
        if (!isHit)
        {
            Debug.Log("攻击未命中！");
            damage = 0;
        }

        // 4. 护盾吸收
        int shieldAbsorb = Mathf.Min((int)damage, this.Shield);
        this.Shield -= shieldAbsorb;
        damage -= shieldAbsorb;

        // 5. 实际扣血
        this.Health -= (int)damage;
        this.Health = Mathf.Max(0, this.Health); // 确保生命值不低于0

        // 6. 吸血效果
        if (damage > 0 && attacker.Lifesteal > 0)
        {
            int healAmount = (int)(damage * attacker.Lifesteal);
            attacker.Health += healAmount;
            attacker.Health = Mathf.Min(attacker.MaxHealth, attacker.Health); // 确保生命值不超过上限
        }

        // 输出日志
        Debug.Log($"攻击者造成伤害: {damage}, 是否暴击: {isCritical}, 护盾吸收: {shieldAbsorb}, 防御者剩余生命: {this.Health}");
    }

}
