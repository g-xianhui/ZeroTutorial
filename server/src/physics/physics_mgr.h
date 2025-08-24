#pragma once

#include <PxPhysicsAPI.h>

class PhysicsMgr
{
public:
    PhysicsMgr() = default;

    int init();
    void fini();

    physx::PxScene* create_px_scene();
    physx::PxScene* load_px_scene(const char* scene_name);

    physx::PxPhysics* get_px_physics() const {
        return px_physics_;
    }

private:
    physx::PxFoundation* px_foundation_ = nullptr;
    physx::PxPhysics* px_physics_ = nullptr;
    physx::PxPvd* px_pvd_ = nullptr;
    physx::PxPvdTransport* px_transport_ = nullptr;
    physx::PxDefaultCpuDispatcher* px_cpu_dispatcher_ = nullptr;

    static physx::PxDefaultErrorCallback px_error_callback;
    static physx::PxDefaultAllocator px_allocator_callback;
};

extern PhysicsMgr G_PhysicsMgr;