using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using UnityEngine;

public class NetSerializer
{
    public static string ReadString(BinaryReader br)
    {
        UInt16 len = br.ReadUInt16();
        byte[] chars = br.ReadBytes(len);
        return System.Text.Encoding.UTF8.GetString(chars);
    }

    public static bool IsGenericList(Type t)
    {
        return (t.IsGenericType && (t.GetGenericTypeDefinition() == typeof(List<>)));
    }

    public static object Read(BinaryReader binaryReader, Type t)
    {
        if (t == typeof(string))
        {
            return ReadString(binaryReader);
        }
        else if (t == typeof(Int32))
        {
            Int32 n = binaryReader.ReadInt32();
            return n;
        }
        else if (t == typeof(UInt32))
        {
            UInt32 n = binaryReader.ReadUInt32();
            return n;
        }
        else if (t == typeof(float))
        {
            float n = binaryReader.ReadSingle();
            return n;
        }
        else if (t == typeof(byte[]))
        {
            UInt32 len = binaryReader.ReadUInt32();
            byte[] chars = binaryReader.ReadBytes((int)len);
            return chars;
        }
        else if (t.IsArray)
        {
            UInt16 len = binaryReader.ReadUInt16();
            Array array = Array.CreateInstance(t.GetElementType()!, len);
            for (int i = 0; i < len; i++)
            {
                object o = Read(binaryReader, t.GetElementType()!);
                array.SetValue(o, i);
            }
            return array;
        }
        else if (IsGenericList(t))
        {
            UInt16 len = binaryReader.ReadUInt16();
            object o = Activator.CreateInstance(t);
            IList array = (IList)o!;

            Type[] typeParameters = t.GetGenericArguments();
            Type elementType = typeParameters[0];

            for (int i = 0; i < len; i++)
            {
                object tmp = Read(binaryReader, elementType!);
                array.Add(tmp);
            }
            return array;
        }
        else if (t.IsClass)
        {
            object ins = Activator.CreateInstance(t);
            MethodInfo method = t.GetMethod("NetSerialize", new[] { typeof(BinaryReader) });
            method.Invoke(ins, new object[] { binaryReader });
            return ins;
        }
        else
        {
            Debug.Assert(false, $"unsupport serialize type: {t}");
            return null;
        }
    }

    public static void Update(BinaryReader binaryReader, ref object obj)
    {
        Type t = obj.GetType();
        if (t == typeof(string))
        {
            obj = ReadString(binaryReader);
        }
        else if (t == typeof(Int32))
        {
            obj = binaryReader.ReadInt32();
        }
        else if (t == typeof(UInt32))
        {
            obj = binaryReader.ReadUInt32();
        }
        else if (t == typeof(float))
        {
            obj = binaryReader.ReadSingle();
        }
        else if (t == typeof(byte[]))
        {
            UInt32 len = binaryReader.ReadUInt32();
            byte[] chars = binaryReader.ReadBytes((int)len);
            obj = chars;
        }
        else if (t.IsArray)
        {
            UInt16 len = binaryReader.ReadUInt16();
            Array array = Array.CreateInstance(t.GetElementType()!, len);
            for (int i = 0; i < len; i++)
            {
                object o = Read(binaryReader, t.GetElementType()!);
                array.SetValue(o, i);
            }
            obj = array;
        }
        else if (IsGenericList(t))
        {
            UInt16 len = binaryReader.ReadUInt16();
            object o = Activator.CreateInstance(t);
            IList array = (IList)o!;

            Type[] typeParameters = t.GetGenericArguments();
            Type elementType = typeParameters[0];

            for (int i = 0; i < len; i++)
            {
                object tmp = Read(binaryReader, elementType!);
                array.Add(tmp);
            }
            obj = array;
        }
        else if (t.IsClass)
        {
            MethodInfo method = t.GetMethod("NetDeltaSerialize", new[] { typeof(BinaryReader) });
            method.Invoke(obj, new object[] { binaryReader });
        }
        else
        {
            Debug.Assert(false, $"unsupport serialize type: {t}");
        }
    }
}