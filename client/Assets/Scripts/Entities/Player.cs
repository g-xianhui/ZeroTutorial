using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class Player : Entity
{
    public string Name;

    private void Awake()
    {
        SimulateMovement simulateMovement = GetComponent<SimulateMovement>();
        if (simulateMovement != null )
        {
            AddComponentByName("MovementComponent", simulateMovement);
        }

        CombatComponent combatComponent = GetComponent<CombatComponent>();
        AddComponentByName("CombatComponent", combatComponent);
    }

    enum DirtyFlag : UInt32
    {
        Name = 1 << 0,
    }

    public override void NetSerialize(BinaryReader br)
    {
        Name = NetSerializer.ReadString(br);
    }

    public override void NetDeltaSerialize(BinaryReader br)
    {
        UInt32 dirtyFlag = br.ReadUInt32();
        if (dirtyFlag != 0)
        {
            if ((dirtyFlag & (UInt32)DirtyFlag.Name) != 0)
            {
                Name = NetSerializer.ReadString(br);
            }
        }
    }
}
