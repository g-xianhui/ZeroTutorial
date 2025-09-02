#include "UnityPhysicsLoader.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern std::string DATA_PATH;

UnityPhysicsLoader::UnityPhysicsLoader() {}

UnityPhysicsLoader::~UnityPhysicsLoader() {}

bool UnityPhysicsLoader::LoadFromFile(const std::string& scenename) {
    std::filesystem::path dataPath{ DATA_PATH };
    std::filesystem::path basePath = dataPath / scenename;
    std::filesystem::path filePath = basePath / "scene.json";
    std::string filename = filePath.string();
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        json j;
        file >> j;

        // 解析重力
        sceneData_.gravity = physx::PxVec3(
            j["gravity"]["x"].get<float>(),
            j["gravity"]["y"].get<float>(),
            j["gravity"]["z"].get<float>()
        );

        // 解析物理体
        for (const auto& bodyJson : j["bodies"]) {
            PhysicsBodyData bodyData;

            bodyData.name = bodyJson["name"].get<std::string>();

            // 使用 PhysX 的向量和四元数
            bodyData.position = physx::PxVec3(
                bodyJson["position"]["x"].get<float>(),
                bodyJson["position"]["y"].get<float>(),
                bodyJson["position"]["z"].get<float>()
            );

            bodyData.rotation = physx::PxQuat(
                bodyJson["rotation"]["x"].get<float>(),
                bodyJson["rotation"]["y"].get<float>(),
                bodyJson["rotation"]["z"].get<float>(),
                bodyJson["rotation"]["w"].get<float>()
            );

            bodyData.scale = physx::PxVec3(
                bodyJson["scale"]["x"].get<float>(),
                bodyJson["scale"]["y"].get<float>(),
                bodyJson["scale"]["z"].get<float>()
            );

            bodyData.isStatic = bodyJson["isStatic"].get<bool>();
            bodyData.mass = bodyJson.value("mass", 1.0f);
            bodyData.drag = bodyJson.value("drag", 0.0f);
            bodyData.angularDrag = bodyJson.value("angularDrag", 0.05f);

            // 解析形状
            if (bodyJson.contains("shapes")) {
                for (const auto& shapeJson : bodyJson["shapes"]) {
                    PhysicsShapeData shapeData;
                    shapeData.shapeType = shapeJson.value("shapeType", 0);

                    shapeData.size = physx::PxVec3(
                        shapeJson["size"]["x"].get<float>(),
                        shapeJson["size"]["y"].get<float>(),
                        shapeJson["size"]["z"].get<float>()
                    );

                    shapeData.center = physx::PxVec3(
                        shapeJson["center"]["x"].get<float>(),
                        shapeJson["center"]["y"].get<float>(),
                        shapeJson["center"]["z"].get<float>()
                    );

                    shapeData.rotation = physx::PxQuat(
                        shapeJson["rotation"]["x"].get<float>(),
                        shapeJson["rotation"]["y"].get<float>(),
                        shapeJson["rotation"]["z"].get<float>(),
                        shapeJson["rotation"]["w"].get<float>()
                    );

                    shapeData.isConvex = shapeJson.value("isConvex", false);
                    shapeData.meshName = shapeJson.value("meshName", "");
                    if (!shapeData.meshName.empty()) {
                        std::filesystem::path meshPath = basePath / (shapeData.meshName + ".obj");
                        shapeData.meshPath = meshPath.string();
                    }

                    bodyData.shapes.push_back(shapeData);
                }
            }

            sceneData_.bodies.push_back(bodyData);
        }

        std::cout << "Loaded " << sceneData_.bodies.size() << " physics bodies" << std::endl;
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

bool UnityPhysicsLoader::CreatePhysXScene(physx::PxPhysics* physics, physx::PxScene* scene) {
    if (!physics || !scene) {
        std::cerr << "Invalid physics or scene pointer" << std::endl;
        return false;
    }

    // 设置重力
    scene->setGravity(sceneData_.gravity);

    // 创建默认材质
    physx::PxMaterial* defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.6f);
    if (!defaultMaterial) {
        std::cerr << "Failed to create default material" << std::endl;
        return false;
    }

    int successCount = 0;
    size_t totalCount = sceneData_.bodies.size();

    // 创建所有物理体
    for (const auto& bodyData : sceneData_.bodies) {
        physx::PxRigidActor* actor = CreateBodyFromData(physics, bodyData, defaultMaterial);
        if (actor) {
            scene->addActor(*actor);
            successCount++;
        }
        else {
            std::cerr << "Failed to create body: " << bodyData.name << std::endl;
        }
    }

    defaultMaterial->release();

    std::cout << "Successfully created " << successCount << " out of " << totalCount << " bodies" << std::endl;
    return successCount > 0;
}

physx::PxRigidActor* UnityPhysicsLoader::CreateBodyFromData(physx::PxPhysics* physics,
    const PhysicsBodyData& bodyData,
    physx::PxMaterial* material) {
    // 创建变换
    physx::PxTransform transform(bodyData.position, bodyData.rotation);

    physx::PxRigidActor* actor = nullptr;

    if (bodyData.isStatic) {
        actor = physics->createRigidStatic(transform);
    }
    else {
        physx::PxRigidDynamic* dynamicActor = physics->createRigidDynamic(transform);
        if (dynamicActor) {
            dynamicActor->setMass(bodyData.mass);
            dynamicActor->setLinearDamping(bodyData.drag);
            dynamicActor->setAngularDamping(bodyData.angularDrag);
        }
        actor = dynamicActor;
    }

    if (!actor) {
        std::cerr << "Failed to create actor for: " << bodyData.name << std::endl;
        return nullptr;
    }

    // 创建并附加所有形状
    for (const auto& shapeData : bodyData.shapes) {
        physx::PxShape* shape = CreateShapeFromData(physics, shapeData, material);
        if (shape) {
            actor->attachShape(*shape);
            shape->release(); // 释放形状引用
        }
    }

    // 设置名称（用于调试）
    actor->setName(bodyData.name.c_str());

    return actor;
}

physx::PxShape* UnityPhysicsLoader::CreateShapeFromData(physx::PxPhysics* physics,
    const PhysicsShapeData& shapeData,
    physx::PxMaterial* material) {
    physx::PxShape* shape = nullptr;

    try {
        switch (shapeData.shapeType) {
        case 0: { // Box
            // Unity 使用全尺寸，PhysX 使用半尺寸
            physx::PxVec3 halfSize = shapeData.size * 0.5f;
            physx::PxBoxGeometry boxGeometry(halfSize);
            shape = physics->createShape(boxGeometry, *material);
            break;
        }

        case 1: { // Sphere
            // Unity 的 sphereCollider.radius 直接对应 PhysX 的半径
            physx::PxSphereGeometry sphereGeometry(shapeData.size.x);
            shape = physics->createShape(sphereGeometry, *material);
            break;
        }

        case 2: { // Capsule
            // Unity: size.x = radius, size.y = height
            // PhysX: radius, halfHeight
            float radius = shapeData.size.x;
            float halfHeight = shapeData.size.y * 0.5f;
            physx::PxCapsuleGeometry capsuleGeometry(radius, halfHeight);
            shape = physics->createShape(capsuleGeometry, *material);
            break;
        }

        case 3: { // Mesh
            shape = CreateMeshShape(physics, shapeData, material);
            break;
        }

        default:
            std::cerr << "Unknown shape type: " << shapeData.shapeType << std::endl;
            break;
        }

        if (shape) {
            // 设置局部变换
            physx::PxTransform localTransform(shapeData.center);
            shape->setLocalPose(localTransform);
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error creating shape: " << e.what() << std::endl;
    }

    return shape;
}

physx::PxShape* UnityPhysicsLoader::CreateMeshShape(physx::PxPhysics* physics, const PhysicsShapeData& shapeData, physx::PxMaterial* material) {
    if (shapeData.meshName.empty()) {
        std::cout << "Mesh path not provided for: " << shapeData.meshName << std::endl;
        return nullptr;
    }

    // 尝试加载网格
    physx::PxTriangleMesh* triangleMesh = LoadTriangleMesh(physics, shapeData.meshPath);
    if (!triangleMesh) {
        std::cerr << "Failed to load triangle mesh: " << shapeData.meshPath << std::endl;
        return nullptr;
    }

    return physics->createShape(physx::PxTriangleMeshGeometry(triangleMesh), *material);
}

physx::PxTriangleMesh* UnityPhysicsLoader::LoadTriangleMesh(physx::PxPhysics* physics,
    const std::string& meshPath) {
    // 创建烹饪工具
    physx::PxCookingParams params(physics->getTolerancesScale());
    params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
    params.meshWeldTolerance = 0.01f;
    physx::PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION,
        physics->getFoundation(),
        params);

    if (!cooking) {
        std::cerr << "Failed to create cooking interface" << std::endl;
        return nullptr;
    }

    // 读取网格数据（这里需要实现具体的网格文件读取）
    physx::PxTriangleMeshDesc meshDesc;
    if (!LoadMeshFromFile(meshPath, meshDesc)) {
        cooking->release();
        return nullptr;
    }

    // 烹饪网格
    physx::PxDefaultMemoryOutputStream writeBuffer;
    if (!cooking->cookTriangleMesh(meshDesc, writeBuffer)) {
        std::cerr << "Failed to cook triangle mesh" << std::endl;
        cooking->release();
        return nullptr;
    }

    physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    physx::PxTriangleMesh* triangleMesh = physics->createTriangleMesh(readBuffer);

    cooking->release();
    return triangleMesh;
}

bool UnityPhysicsLoader::LoadMeshFromFile(const std::string& meshPath, physx::PxTriangleMeshDesc& meshDesc) {
    auto iter = mesh_triangle_map_.find(meshPath);
    if (iter == mesh_triangle_map_.end()) {
        std::ifstream file(meshPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open mesh file: " << meshPath << std::endl;
            return false;
        }

        MeshTriangleData mesh_triangle_data;

        // 解析 OBJ 文件或其他格式
        std::string line;
        while (std::getline(file, line)) {
            if (line.substr(0, 2) == "v ") {
                // 解析顶点
                std::istringstream s(line.substr(2));
                float x, y, z;
                s >> x >> y >> z;
                mesh_triangle_data.vertices.push_back(physx::PxVec3(x, y, z));
            }
            else if (line.substr(0, 2) == "f ") {
                // 解析面
                std::istringstream s(line.substr(2));
                physx::PxU32 i1, i2, i3;
                s >> i1 >> i2 >> i3;
                mesh_triangle_data.indices.push_back(i1 - 1); // OBJ 索引从1开始
                mesh_triangle_data.indices.push_back(i2 - 1);
                mesh_triangle_data.indices.push_back(i3 - 1);
            }
        }

        mesh_triangle_map_[meshPath] = mesh_triangle_data;
    }

    MeshTriangleData& data = mesh_triangle_map_[meshPath];

    // 填充网格描述
    meshDesc.points.count = static_cast<physx::PxU32>(data.vertices.size());
    meshDesc.points.stride = sizeof(physx::PxVec3);
    meshDesc.points.data = data.vertices.data();

    meshDesc.triangles.count = static_cast<physx::PxU32>(data.indices.size() / 3);
    meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
    meshDesc.triangles.data = data.indices.data();

    return true;
}

void UnityPhysicsLoader::DebugMeshValidation(const physx::PxTriangleMeshDesc& meshDesc) {
    std::cout << "=== Mesh Validation Debug ===" << std::endl;
    std::cout << "Points count: " << meshDesc.points.count << std::endl;
    std::cout << "Points stride: " << meshDesc.points.stride << std::endl;
    std::cout << "Triangles count: " << meshDesc.triangles.count << std::endl;
    std::cout << "Triangles stride: " << meshDesc.triangles.stride << std::endl;

    if (meshDesc.points.count > 0 && meshDesc.points.data) {
        const physx::PxVec3* vertices = static_cast<const physx::PxVec3*>(meshDesc.points.data);
        std::cout << "First 3 vertices:" << std::endl;
        for (int i = 0; i < std::min(3, (int)meshDesc.points.count); ++i) {
            std::cout << "  v" << i << ": (" << vertices[i].x << ", "
                << vertices[i].y << ", " << vertices[i].z << ")" << std::endl;
        }
    }

    if (meshDesc.triangles.count > 0 && meshDesc.triangles.data) {
        const physx::PxU32* indices = static_cast<const physx::PxU32*>(meshDesc.triangles.data);
        std::cout << "First 3 triangles:" << std::endl;
        for (int i = 0; i < std::min(3, (int)meshDesc.triangles.count); ++i) {
            std::cout << "  t" << i << ": (" << indices[i * 3] << ", "
                << indices[i * 3 + 1] << ", " << indices[i * 3 + 2] << ")" << std::endl;
        }
    }

    // 检查内存对齐
    std::cout << "Points data alignment: " << ((uintptr_t)meshDesc.points.data % alignof(physx::PxVec3)) << std::endl;
    std::cout << "Triangles data alignment: " << ((uintptr_t)meshDesc.triangles.data % alignof(physx::PxU32)) << std::endl;
    std::cout << "============================" << std::endl;
}