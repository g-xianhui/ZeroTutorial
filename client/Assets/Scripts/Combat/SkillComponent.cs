using Google.Protobuf;
using System;
using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public class SkillComponent : MonoBehaviour
{
    private List<int> skills = new List<int>();
    private ISkill runningSkill = null;

    // Start is called before the first frame update
    void Start()
    {
        skills.Add(1);
    }

    // Update is called once per frame
    void Update()
    {
        if (runningSkill != null && runningSkill.IsActive)
        {
            runningSkill.Update(this);
        }
    }

    public void CastSkill(int skillIndex)
    {
        // 暂定同时只能运行一个技能
        if (runningSkill != null && runningSkill.IsActive)
            return;

        if (skillIndex < 0 || skillIndex >= skills.Count)
            return;

        int skillId = skills[skillIndex];

        Type skillType = Type.GetType($"Skill_{skillId}");
        if (skillType == null)
        {
            throw new ArgumentException($"Skill_{skillId} not found!");
        }

        // 创建实例
        runningSkill = (ISkill)Activator.CreateInstance(skillType);
        runningSkill.Execute(this);

        if (!NetworkManager.Instance.IsOfflineMode)
        {
            SpaceService.SkillAttack req = new SpaceService.SkillAttack
            {
                SkillId = skillId,
            };
            NetworkManager.Instance.Send("skill_attack", req.ToByteArray());
        }
    }

    public void StopSkill()
    {
        if (runningSkill != null && runningSkill.IsActive)
            runningSkill.Stop(this);
    }
}
