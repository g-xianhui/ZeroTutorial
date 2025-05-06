using System.Collections;
using System.Collections.Generic;
using System;
using System.IO;
using UnityEngine;

public class Entity : MonoBehaviour
{
    public int CharacterId;
    public int Eid;
    public string Name;

    enum DirtyFlag : byte
    {
        Name = 1 << 0,
    }

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void NetSerialize(byte[] data)
    {
        using (MemoryStream mem = new MemoryStream(data)) {
            using (BinaryReader br = new BinaryReader(mem)) {
                Eid = br.ReadInt32();
                Name = NetSerializer.ReadString(br);

                while (mem.Position < mem.Length) {
                    string componentName = NetSerializer.ReadString(br);
                    if (componentName == "CombatComponent") {
                        CombatComponent comp = GetComponent<CombatComponent>();
                        if (comp != null) {
                            comp.NetSerialize(br);
                        }
                    }
                }
             }
        }
    }

    public void NetDeltaSerialize(byte[] data)
    {
        using (MemoryStream mem = new MemoryStream(data))
        {
            using (BinaryReader br = new BinaryReader(mem))
            {
                byte dirtyFlag = br.ReadByte();
                if (dirtyFlag != 0)
                {
                    if ((dirtyFlag & (byte)DirtyFlag.Name) != 0)
                    {
                        Name = NetSerializer.ReadString(br);
                    }
                }

                while (mem.Position < mem.Length)
                {
                    string componentName = NetSerializer.ReadString(br);
                    if (componentName == "CombatComponent")
                    {
                        CombatComponent comp = GetComponent<CombatComponent>();
                        if (comp != null)
                        {
                            comp.NetDeltaSerialize(br);
                        }
                    }
                }
            }
        }
    }
}
