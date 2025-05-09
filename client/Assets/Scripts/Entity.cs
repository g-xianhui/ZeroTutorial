using System.Collections;
using System.Collections.Generic;
using System;
using System.IO;
using UnityEngine;

public class Entity : MonoBehaviour
{
    public int Eid;

    private Dictionary<string, IComponent> _components = new Dictionary<string, IComponent>();

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void AddComponentByName(string name, IComponent component)
    {
        _components.Add(name, component);
    }

    public IComponent GetComponentByName(string name)
    {
        return _components.GetValueOrDefault(name);
    }

    public virtual void NetSerialize(BinaryReader br)
    {

    }

    public virtual void NetDeltaSerialize(BinaryReader br)
    {

    }

    public void EntityNetSerialize(byte[] data)
    {
        using (MemoryStream mem = new MemoryStream(data)) {
            using (BinaryReader br = new BinaryReader(mem)) {
                Eid = br.ReadInt32();

                NetSerialize(br);

                while (mem.Position < mem.Length) {
                    string componentName = NetSerializer.ReadString(br);
                    IComponent component = _components.GetValueOrDefault(componentName);
                    if (component != null)
                    {
                        component.NetSerialize(br);
                    }
                }
             }
        }
    }

    public void EntityNetDeltaSerialize(byte[] data)
    {
        using (MemoryStream mem = new MemoryStream(data))
        {
            using (BinaryReader br = new BinaryReader(mem))
            {
                NetDeltaSerialize(br);

                while (mem.Position < mem.Length)
                {
                    string componentName = NetSerializer.ReadString(br);
                    IComponent component = _components.GetValueOrDefault(componentName);
                    if (component != null)
                    {
                        component.NetDeltaSerialize(br);
                    }
                }
            }
        }
    }
}
