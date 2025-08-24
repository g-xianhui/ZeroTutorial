#pragma once
#include <string>
#include <vector>
#include <PxPhysicsAPI.h>

struct PhysicsShapeData {
    int shapeType; // 0=Box, 1=Sphere, 2=Capsule, 3=Mesh
    physx::PxVec3 size;
    physx::PxVec3 center;
    physx::PxQuat rotation;

    std::string meshName;
    std::string meshPath;
    bool isConvex;
};

struct PhysicsBodyData {
    std::string name;
    physx::PxVec3 position;
    physx::PxQuat rotation;
    physx::PxVec3 scale;
    bool isStatic;
    float mass;
    float drag;
    float angularDrag;
    std::vector<PhysicsShapeData> shapes;
};

struct PhysicsSceneData {
    physx::PxVec3 gravity;
    std::vector<PhysicsBodyData> bodies;
    std::string version;
    std::string exportTime;
};