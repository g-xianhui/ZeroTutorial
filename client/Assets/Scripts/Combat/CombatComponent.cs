using Google.Protobuf;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Animator), typeof(CharacterMovement), typeof(AttrSet))]
public class CombatComponent : MonoBehaviour
{
    private Animator _anim;
    private CharacterMovement _characterMovement;
    private AttrSet _attrSet;
    private SkillComponent _skillComponent;
    private NetworkComponent _networkComponent;

    private int comboSeq = 0;
    private bool comboPressed = false;
    private float nextNormalAttackTime = 0;

    public List<string> ComboClips = new List<string>();
    public bool EnableNormalAttack { get; set; } = true;
    public bool EnableComboAttack { get; set; } = false;

    // Start is called before the first frame update
    void Start()
    {
        _anim = GetComponent<Animator>();
        _characterMovement = GetComponent<CharacterMovement>();
        _attrSet = GetComponent<AttrSet>();
        _skillComponent = GetComponent<SkillComponent>();
        _networkComponent = GetComponent<NetworkComponent>();
    }

    // Update is called once per frame
    void Update()
    {
    }

    public void NormalAttack()
    {
        if (Time.time < nextNormalAttackTime)
            return;

        if (!EnableNormalAttack)
        {
            if (EnableComboAttack)
            {
                comboPressed = true;
            }
            return;
        }

        float interval = 1.0f / (_attrSet.AttackSpeed + _attrSet.AdditionalAttackSpeed);
        nextNormalAttackTime = Time.time + interval;
        float playRate = (_attrSet.AttackSpeed + _attrSet.AdditionalAttackSpeed) / _attrSet.AttackSpeed;
        _anim.speed = playRate;

        string animClip = ComboClips[comboSeq];
        _anim.CrossFade(animClip, 0.2f * playRate);

        if (!NetworkManager.Instance.IsOfflineMode)
        {
            SpaceService.NormalAttack req = new SpaceService.NormalAttack
            {
                Combo = comboSeq,
            };
            NetworkManager.Instance.Send("normal_attack", req.ToByteArray());
        }

        EnableNormalAttack = false;
        comboSeq = (comboSeq + 1) % ComboClips.Count;
    }


    public void SkillAttack(int skillIndex)
    {
        _skillComponent.CastSkill(skillIndex);
    }

    public void AN_EnableNormalAttack(int enable)
    {
        EnableNormalAttack = enable > 0;
    }

    public void AN_EnableMovement(int enable)
    {
        // simulator的移动状态由网络接管
        if (_networkComponent != null && _networkComponent.NetRole == ENetRole.Simulate)
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
}
