using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Skill_1 : ISkill
{
    
    public override void Execute(SkillComponent skillComponent)
    {
        CharacterMovement characterMovement = skillComponent.GetComponent<CharacterMovement>();
        characterMovement.EnableMovement = false;
        characterMovement.IgnoreGravity = true;

        FootIK footIK = skillComponent.GetComponent<FootIK>();
        footIK.UseFootIK = false;

        Animator anim = skillComponent.GetComponent<Animator>();
        anim.CrossFade("Skill1", 0.2f);
    }

    public override void Stop(SkillComponent skillComponent)
    {
        base.Stop(skillComponent);
        CharacterMovement characterMovement = skillComponent.GetComponent<CharacterMovement>();
        characterMovement.EnableMovement = true;
        characterMovement.IgnoreGravity = false;

        FootIK footIK = skillComponent.GetComponent<FootIK>();
        footIK.UseFootIK = true;
    }
}
