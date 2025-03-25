using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SkillComponent : MonoBehaviour
{
    private List<int> skills = new List<int>();

    // Start is called before the first frame update
    void Start()
    {
        if (NetworkManager.Instance.IsOfflineMode)
        {
            skills.Add(1);
        }
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void CastSkill(int skillIndex)
    {
        if (skillIndex < 0 || skillIndex >= skills.Count)
            return;

        int skillId = skills[skillIndex];

        Type skillType = Type.GetType($"Skill_{skillId}");
        if (skillType == null)
        {
            throw new ArgumentException($"Skill_{skillId} not found!");
        }

        // ´´½¨ÊµÀý
        ISkill skill = (ISkill)Activator.CreateInstance(skillType);
        skill.Execute();
    }
}
