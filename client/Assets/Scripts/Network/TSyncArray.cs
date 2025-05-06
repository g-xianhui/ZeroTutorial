using Google.Protobuf.Collections;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;

enum SyncArrayOperation : Byte
{
    update,
    push_back,
    pop_back,
    insert,
    erase,
    clear,
    resize,
    replace
};

public class TSyncArray<T>
{
    private List<T> vec = new List<T>();

    public T this[int index]
    {
        get => vec[index];
        set => vec[index] = value;
    }

    public IEnumerator<T> GetEnumerator() => vec.GetEnumerator();

    public int Count => vec.Count;

    public void Add(T item) => vec.Add(item);

    public void NetSerialize(BinaryReader br)
    {
        Type t = typeof(T);
        IList array = (IList)vec;

        UInt16 len = br.ReadUInt16();
        for (int i = 0; i < len; i++)
        {
            object element = NetSerializer.Read(br, t);
            array.Add(element);
        }
    }

    public void NetDeltaSerialize(BinaryReader br)
    {
        Type t = typeof(T);
        IList array = (IList)vec;

        UInt32 dirtySize = br.ReadUInt32();
        if (dirtySize > 0)
        {
            long endPos = br.BaseStream.Position + dirtySize;
            while (br.BaseStream.Position < endPos)
            {
                Byte n = br.ReadByte();
                SyncArrayOperation op = (SyncArrayOperation)n;
                switch (op)
                {
                    case SyncArrayOperation.update:
                        {
                            UInt16 pos = br.ReadUInt16();
                            object element = array[pos];
                            NetSerializer.Update(br, ref element);
                            array[pos] = element;
                        }
                        break;
                    case SyncArrayOperation.push_back:
                        {
                            object element = NetSerializer.Read(br, t);
                            array.Add(element);
                        }
                        break;
                    case SyncArrayOperation.pop_back:
                        {
                            array.RemoveAt(array.Count - 1);
                        }
                        break;
                    case SyncArrayOperation.insert:
                        {
                            UInt16 pos = br.ReadUInt16();
                            object element = NetSerializer.Read(br, t);
                            array.Insert(pos, element);
                        }
                        break;
                    case SyncArrayOperation.erase:
                        {
                            UInt16 pos = br.ReadUInt16();
                            array.RemoveAt(pos);
                        }
                        break;
                    case SyncArrayOperation.clear:
                        {
                            vec.Clear();
                        }
                        break;
                    case SyncArrayOperation.resize:
                        {
                            UInt16 newSize = br.ReadUInt16();
                            if (vec.Count < newSize)
                            {
                                vec.AddRange(Enumerable.Repeat(default(T), newSize - vec.Count));
                            }
                            else if (vec.Count > newSize)
                            {
                                vec.RemoveRange(newSize, vec.Count - newSize);
                            }
                        }
                        break;
                    case SyncArrayOperation.replace:
                        {
                            TSyncArray<T> newArray = new TSyncArray<T>();
                            newArray.NetSerialize(br);
                            vec = newArray.vec;
                        }
                        break;
                    default:
                        Debug.Assert(false, $"unknown array operation: {n}");
                        break;
                }
            }
        }
    }
}
