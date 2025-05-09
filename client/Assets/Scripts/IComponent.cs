using SpaceService;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class IComponent : MonoBehaviour
{
    public virtual void NetSerialize(BinaryReader br)
    {
    }

    public virtual void NetDeltaSerialize(BinaryReader br)
    {
    }
}
