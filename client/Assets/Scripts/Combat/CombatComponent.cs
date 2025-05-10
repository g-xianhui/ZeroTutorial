using Google.Protobuf;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;

public class SkillInfo
{
    public int SkillId;
    public string AnimatorState;
    public int CostMana = 0;
    public int CoolDown = 0;
    public int NextCastTime = 0;
    // 本地先行
    public bool LocalPredicted = false;

    enum DirtyFlag : byte
    {
        SkillId = 1,
        AnimatorState = 2,
        CostMana = 4,
        CoolDown = 8,
        NextCastTime = 16,
        LocalPredicted = 32
    }

    public void NetSerialize(BinaryReader br)
    {
        SkillId = br.ReadInt32();
        AnimatorState = NetSerializer.ReadString(br);
        CostMana = br.ReadInt32();
        CoolDown = br.ReadInt32();
        NextCastTime = br.ReadInt32();
        LocalPredicted = br.ReadBoolean();
    }

    public void NetDeltaSerialize(BinaryReader br)
    {
        byte dirtyFlag = br.ReadByte();
        if (dirtyFlag != 0)
        {
            if ((dirtyFlag & (byte)DirtyFlag.SkillId) != 0)
            {
                SkillId = br.ReadInt32();
            }

            if ((dirtyFlag & (UInt16)DirtyFlag.AnimatorState) != 0)
            {
                AnimatorState = NetSerializer.ReadString(br);
            }

            if ((dirtyFlag & (UInt16)DirtyFlag.CostMana) != 0)
            {
                CostMana = br.ReadInt32();
            }

            if ((dirtyFlag & (UInt16)DirtyFlag.CoolDown) != 0)
            {
                CoolDown = br.ReadInt32();
            }

            if ((dirtyFlag & (UInt16)DirtyFlag.NextCastTime) != 0)
            {
                NextCastTime = br.ReadInt32();
            }

            if ((dirtyFlag & (UInt16)DirtyFlag.LocalPredicted) != 0)
            {
                LocalPredicted = br.ReadBoolean();
            }
        }
    }
}

[RequireComponent(typeof(Animator))]
[RequireComponent(typeof(CharacterMovement))]
[RequireComponent(typeof(NetworkComponent))]
public class CombatComponent : IComponent
{
    private Animator _anim;
    private CharacterMovement _characterMovement;
    private NetworkComponent _networkComponent;

    private AttrSet attrSet = new AttrSet();
    private TSyncArray<SkillInfo> skillInfos = new TSyncArray<SkillInfo>();

    // 普攻相关属性
    private int comboSeq = 0;
    private bool comboPressed = false;
    private float nextNormalAttackTime = 0;
    public List<string> ComboClips = new List<string>();
    public bool EnableNormalAttack { get; set; } = true;
    public bool EnableComboAttack { get; set; } = false;

    [SerializeField]
    private GameObject headUIMountPoint;
    private HeadUI headUI = null;

    private void Awake()
    {
        _anim = GetComponent<Animator>();
        _characterMovement = GetComponent<CharacterMovement>();
        _networkComponent = GetComponent<NetworkComponent>();

        CreateHeadUI();
    }

    // Start is called before the first frame update
    void Start()
    {
        initSkill();
    }

    // Update is called once per frame
    void Update()
    {
        headUI.ShowAt(headUIMountPoint.transform.position);
    }

    private void OnDestroy()
    {
        try
        {
            Destroy(headUI.gameObject);
        }
        catch
        {
        }
    }

    public override void NetSerialize(BinaryReader br)
    {
        attrSet.NetSerialize(br);

        if (!_networkComponent.IsSimulate())
            skillInfos.NetSerialize(br);

        UpdateHeadUI();
    }

    public override void NetDeltaSerialize(BinaryReader br)
    {
        attrSet.NetDeltaSerialize(br);

        if (!_networkComponent.IsSimulate())
            skillInfos.NetDeltaSerialize(br);

        UpdateHeadUI();
    }

    void CreateHeadUI()
    {
        GameObject prefab = Resources.Load<GameObject>("UI/HeadUI");
        GameObject headUIGameObject = GameObject.Instantiate(prefab);
        headUIGameObject.transform.SetParent(FindObjectOfType<Canvas>().transform, false);
        headUI = headUIGameObject.AddComponent<HeadUI>();
    }

    void UpdateHeadUI()
    {
        headUI.UpdateHeathSlider((attrSet.Health * 1.0f) / attrSet.MaxHealth);
        headUI.UpdateManaSlider((attrSet.Mana * 1.0f) / attrSet.MaxMana);
    }

    public AttrSet GetAttrSet()
    {
        return attrSet;
    }

    public SkillInfo GetSkillInfo(int skillId)
    {
        foreach (var skillInfo in skillInfos)
        {
            if (skillInfo.SkillId == skillId)
                return skillInfo;
        }

        return null;
    }

    private void initSkill()
    {
        if (!NetworkManager.Instance.IsOfflineMode)
            return;

        // TODO 配置
        {
            // 技能1
            SkillInfo skillInfo = new SkillInfo
            {
                SkillId = 1,
                AnimatorState = "Skill1",
                CoolDown = 5000,
                CostMana = 2,
            };
            skillInfos.Add(skillInfo);
        }
    }

    public bool CanCastSkill(int skillIndex)
    {
        if (skillIndex < 0 || skillIndex >= skillInfos.Count)
            return false;

        SkillInfo skillInfo = skillInfos[skillIndex];

        if (NetworkManager.Instance.GetServerTime() < skillInfo.NextCastTime)
            return false;

        if (attrSet.Mana < skillInfo.CostMana)
            return false;

        return true;
    }

    public void CastSkill(int skillIndex)
    {
        if (!CanCastSkill(skillIndex)) return;

        SkillInfo skillInfo = skillInfos[skillIndex];
        int skillId = skillInfo.SkillId;

        // 本地先行
        if (skillInfo.LocalPredicted)
        {
            // 目前回滚仅涉及cd和mana，因此服务端在收到请求时，无论是否成功，都下发最新的cd与mana即可。
            skillInfo.NextCastTime = NetworkManager.Instance.GetServerTime() + skillInfo.CoolDown;
            attrSet.Mana -= skillInfo.CostMana;
            _anim.CrossFade(skillInfo.AnimatorState, 0.2f);
        }

        SpaceService.SkillAttack req = new SpaceService.SkillAttack
        {
            SkillId = skillId,
        };
        NetworkManager.Instance.Send("skill_attack", req.ToByteArray());
    }

    public void NormalAttack()
    {
        if (NetworkManager.Instance.GetServerTime() < nextNormalAttackTime)
            return;

        if (!EnableNormalAttack)
        {
            if (EnableComboAttack)
            {
                comboPressed = true;
            }
            return;
        }

        float interval = 1.0f / (attrSet.AttackSpeed + attrSet.AdditionalAttackSpeed);
        nextNormalAttackTime = Time.time + interval;

        float playRate = (attrSet.AttackSpeed + attrSet.AdditionalAttackSpeed) / attrSet.AttackSpeed;
        _anim.speed = playRate;

        string animClip = ComboClips[comboSeq];
        _anim.CrossFade(animClip, 0.2f * playRate);

        // 普攻默认先行，并且不做回滚，毕竟间隔很短，也不耗蓝
        SpaceService.NormalAttack req = new SpaceService.NormalAttack
        {
            Combo = comboSeq,
        };
        NetworkManager.Instance.Send("normal_attack", req.ToByteArray());

        EnableNormalAttack = false;
        comboSeq = (comboSeq + 1) % ComboClips.Count;
    }

    public void SkillAttack(int skillIndex)
    {
        CastSkill(skillIndex);
    }

    // 动画过渡会把结尾的事件回调忽略掉
    public void AN_EnableNormalAttack(int enable)
    {
        EnableNormalAttack = enable > 0;
    }

    public void AN_EnableMovement(int enable)
    {
        // simulator的移动状态由网络接管
        if (_networkComponent.NetRole == ENetRole.Simulate)
            return;
        _characterMovement.EnableMovement = enable > 0;
    }

    public void AN_EnableCombo(int enable)
    {
        EnableComboAttack = enable > 0;
    }

    public void AN_CheckCombo()
    {
        EnableNormalAttack = true;
        if (comboPressed)
        {
            NormalAttack();
            comboPressed = false;
        }
    }

    public void AN_NormalAttackEnd()
    {
        _anim.speed = 1;
        AN_EnableMovement(1);
    }

    public void TakeDamage(int damage)
    {
        _anim.CrossFade("BeHit", 0.2f);
    }
}
