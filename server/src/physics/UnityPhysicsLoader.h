#pragma once
#include "PhysicsData.h"
#include <PxPhysicsAPI.h>
#include <string>
#include <memory>
#include <map>

struct MeshTriangleData {
    std::vector<physx::PxVec3> vertices;
    std::vector<physx::PxU32> indices;
};

class UnityPhysicsLoader {
public:
    UnityPhysicsLoader();
    ~UnityPhysicsLoader();

    bool LoadFromFile(const std::string& scenename);
    bool CreatePhysXScene(physx::PxPhysics* physics, physx::PxScene* scene);

    const PhysicsSceneData& GetSceneData() const { return sceneData_; }

private:
    physx::PxShape* CreateMeshShape(physx::PxPhysics* physics, const PhysicsShapeData& shapeData, physx::PxMaterial* material);
    physx::PxTriangleMesh* LoadTriangleMesh(physx::PxPhysics* physics, const std::string& meshPath);
    bool LoadMeshFromFile(const std::string& meshPath, physx::PxTriangleMeshDesc& meshDesc);

    physx::PxShape* CreateShapeFromData(physx::PxPhysics* physics,
        const PhysicsShapeData& shapeData,
        physx::PxMaterial* material);

    physx::PxRigidActor* CreateBodyFromData(physx::PxPhysics* physics,
        const PhysicsBodyData& bodyData,
        physx::PxMaterial* material);

    void DebugMeshValidation(const physx::PxTriangleMeshDesc& meshDesc);

private:
    PhysicsSceneData sceneData_;
    std::map<std::string, MeshTriangleData> mesh_triangle_map_;
};