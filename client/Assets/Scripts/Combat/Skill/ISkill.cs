using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class ISkill
{
    public bool IsActive { get; set; } = true;

    public abstract void Execute(SkillComponent skillComponent);

    public virtual void Update(SkillComponent skillComponent) { }

    public virtual void Stop(SkillComponent skillComponent) { IsActive = false; }
}
