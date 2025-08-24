#pragma once
#include "PhysicsData.h"
#include <PxPhysicsAPI.h>
#include <string>
#include <memory>

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

private:
    PhysicsSceneData sceneData_;

    physx::PxShape* CreateShapeFromData(physx::PxPhysics* physics,
        const PhysicsShapeData& shapeData,
        physx::PxMaterial* material);

    physx::PxRigidActor* CreateBodyFromData(physx::PxPhysics* physics,
        const PhysicsBodyData& bodyData,
        physx::PxMaterial* material);
};