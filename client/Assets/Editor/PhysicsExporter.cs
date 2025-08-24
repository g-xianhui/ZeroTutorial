using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.IO;
using System;
using UnityEngine.SceneManagement;

[System.Serializable]
public class PhysicsShapeData
{
    public int shapeType; // 0=Box, 1=Sphere, 2=Capsule, 3=Mesh
    public Vector3 size;
    public Vector3 center;
    public Quaternion rotation;
    public string meshName;
    public bool isConvex;
}

[System.Serializable]
public class PhysicsBodyData
{
    public string name;
    public Vector3 position;
    public Quaternion rotation;
    public Vector3 scale;
    public bool isStatic;
    public float mass;
    public float drag;
    public float angularDrag;
    public PhysicsShapeData[] shapes;
    public string meshPath; // 用于网格碰撞体
}

[System.Serializable]
public class PhysicsSceneData
{
    public Vector3 gravity = new Vector3(0, -9.81f, 0);
    public List<PhysicsBodyData> bodies = new List<PhysicsBodyData>();
    public string version = "1.0";
    public DateTime exportTime;
}

public class PhysicsExporter : Editor
{
    public static string exportPath = "PhysicsData/unity_physics_export.json";
    public static bool includeInactiveObjects = false;
    public static bool exportMeshColliders = true;

    [MenuItem("ExportScene/ExportPhysx")]
    static void ExportPhysicsData(MenuCommand command)
    {
        string savePath = GetSavePath();
        if (string.IsNullOrEmpty(savePath)) return;

        PhysicsSceneData sceneData = new PhysicsSceneData();
        sceneData.exportTime = DateTime.Now;
        sceneData.gravity = Physics.gravity;

        // 收集所有包含碰撞体的物体
        Collider[] allColliders = FindObjectsOfType<Collider>(includeInactiveObjects);

        foreach (Collider collider in allColliders)
        {
            Rigidbody rb = collider.GetComponent<Rigidbody>();
            PhysicsBodyData bodyData = new PhysicsBodyData();

            bodyData.name = collider.gameObject.name;
            bodyData.position = collider.transform.position;
            bodyData.rotation = collider.transform.rotation;
            bodyData.scale = collider.transform.lossyScale;
            bodyData.isStatic = (rb == null) || rb.isKinematic;

            if (rb != null)
            {
                bodyData.mass = rb.mass;
                bodyData.drag = rb.drag;
                bodyData.angularDrag = rb.angularDrag;
            }

            // 处理碰撞体形状
            bodyData.shapes = ProcessCollider(collider, savePath);

            sceneData.bodies.Add(bodyData);
        }

        // 导出为JSON
        string json = JsonUtility.ToJson(sceneData, true);
        File.WriteAllText(Path.Combine(savePath, "scene.json"), json);

        Debug.Log($"Physics data exported to: {savePath}");
        Debug.Log($"Exported {sceneData.bodies.Count} physics bodies");
    }

    static PhysicsShapeData[] ProcessCollider(Collider collider, string savePath)
    {
        List<PhysicsShapeData> shapes = new List<PhysicsShapeData>();

        if (collider is BoxCollider boxCollider)
        {
            shapes.Add(new PhysicsShapeData()
            {
                shapeType = 0,
                size = Vector3.Scale(boxCollider.size, collider.transform.lossyScale),
                center = boxCollider.center,
                rotation = Quaternion.identity
            });
        }
        else if (collider is SphereCollider sphereCollider)
        {
            float maxScale = Mathf.Max(collider.transform.lossyScale.x,
                                     collider.transform.lossyScale.y,
                                     collider.transform.lossyScale.z);
            shapes.Add(new PhysicsShapeData()
            {
                shapeType = 1,
                size = new Vector3(sphereCollider.radius * maxScale, 0, 0),
                center = sphereCollider.center,
                rotation = Quaternion.identity
            });
        }
        else if (collider is CapsuleCollider capsuleCollider)
        {
            shapes.Add(new PhysicsShapeData()
            {
                shapeType = 2,
                size = new Vector3(capsuleCollider.radius, capsuleCollider.height, 0),
                center = capsuleCollider.center,
                rotation = GetCapsuleRotation(capsuleCollider.direction)
            });
        }
        else if (collider is MeshCollider meshCollider && exportMeshColliders)
        {
            // 网格碰撞体需要特殊处理
            //shapes.Add(new PhysicsShapeData()
            //{
            //    shapeType = 3,
            //    size = collider.transform.lossyScale,
            //    center = Vector3.zero,
            //    rotation = Quaternion.identity
            //});
            shapes.Add(CreateMeshShape(meshCollider, savePath));
        }

        return shapes.ToArray();
    }

    static PhysicsShapeData CreateMeshShape(MeshCollider meshCollider, string savePath)
    {
        PhysicsShapeData shapeData = new PhysicsShapeData()
        {
            shapeType = 3, // Mesh
            size = meshCollider.transform.lossyScale,
            center = Vector3.zero,
            rotation = Quaternion.identity
        };

        // 保存网格信息
        if (meshCollider.sharedMesh != null)
        {
            shapeData.meshName = meshCollider.sharedMesh.name;
            shapeData.isConvex = meshCollider.convex;
            ExportMeshData(meshCollider.sharedMesh, savePath);
        }

        return shapeData;
    }

    static string ExportMeshData(Mesh mesh, string savePath)
    {
        string meshPath = Path.Combine(savePath, $"{mesh.name}.obj");

        // 简化：实际项目中应该使用更好的网格导出方式
        // 这里只是示例，实际应该导出为 PhysX 支持的格式
        ExportMeshToObj(mesh, meshPath);

        return meshPath;
    }

    static void ExportMeshToObj(Mesh mesh, string path)
    {
        using (StreamWriter sw = new StreamWriter(path))
        {
            sw.WriteLine("# Exported mesh for PhysX");
            sw.WriteLine($"# Vertices: {mesh.vertices.Length}");
            sw.WriteLine($"# Triangles: {mesh.triangles.Length / 3}");

            // 导出顶点
            foreach (Vector3 vertex in mesh.vertices)
            {
                sw.WriteLine($"v {vertex.x} {vertex.y} {vertex.z}");
            }

            // 导出面
            for (int i = 0; i < mesh.triangles.Length; i += 3)
            {
                int i1 = mesh.triangles[i] + 1;
                int i2 = mesh.triangles[i + 1] + 1;
                int i3 = mesh.triangles[i + 2] + 1;
                sw.WriteLine($"f {i1} {i2} {i3}");
            }
        }

        Debug.Log($"Mesh exported to: {path}");
    }


    static Quaternion GetCapsuleRotation(int direction)
    {
        switch (direction)
        {
            case 0: return Quaternion.Euler(0, 0, 90); // X-axis
            case 1: return Quaternion.identity;        // Y-axis
            case 2: return Quaternion.Euler(90, 0, 0); // Z-axis
            default: return Quaternion.identity;
        }
    }

    private static string GetSavePath()
    {
        string dataPath = Application.dataPath;
        string dir = dataPath.Substring(0, dataPath.LastIndexOf("/"));
        string sceneName = SceneManager.GetActiveScene().name;
        string defaultName = sceneName + "_physics";
        return EditorUtility.SaveFolderPanel("Export physics scene", dir, defaultName);
    }
}