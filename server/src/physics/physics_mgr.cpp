#include "physics_mgr.h"
#include "UnityPhysicsLoader.h"

#include <spdlog/spdlog.h>

physx::PxDefaultErrorCallback PhysicsMgr::px_error_callback;
physx::PxDefaultAllocator PhysicsMgr::px_allocator_callback;

int PhysicsMgr::init()
{
    px_foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, px_allocator_callback, px_error_callback);
    if (!px_foundation_) {
        spdlog::error("PxCreateFoundation failed");
        return 1;
    }

#ifdef ENABLE_PVD
    px_pvd_ = PxCreatePvd(*px_foundation_);
    if (!px_pvd_) {
        spdlog::error("PxCreatePvd failed");
        return 1;
    }
    px_transport_ = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    if (!px_transport_) {
        spdlog::error("PxDefaultPvdSocketTransportCreate failed");
        return 1;
    }

    if (!px_pvd_->connect(*px_transport_, physx::PxPvdInstrumentationFlag::eALL)) {
        spdlog::error("connect pvd failed");
        return 1;
    }
#endif

    px_physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *px_foundation_, physx::PxTolerancesScale(), false, px_pvd_);
    if (!px_physics_) {
        spdlog::error("PxCreatePhysics failed!");
        return 1;
    }

    px_cpu_dispatcher_ = physx::PxDefaultCpuDispatcherCreate(1);
    if (!px_cpu_dispatcher_) {
        spdlog::error("PxDefaultCpuDispatcherCreate failed!");
        return 1;
    }


    if (!PxInitExtensions(*px_physics_, px_pvd_)) {
        spdlog::error("PxInitExtensions failed!");
        return 1;
    }

    return 0;
}

void PhysicsMgr::fini()
{
    if (px_physics_)
        px_physics_->release();

#ifdef ENABLE_PVD
    if (px_pvd_) {
        if(px_pvd_->isConnected())
            px_pvd_->disconnect();
        px_pvd_->release();
    }

    if (px_transport_) {
        px_transport_->release();
    }
#endif

    if (px_cpu_dispatcher_) {
        px_cpu_dispatcher_->release();
    }
    
    if (px_foundation_)
        px_foundation_->release();
}

physx::PxScene* PhysicsMgr::create_px_scene()
{
    physx::PxSceneDesc default_scene_desc = physx::PxSceneDesc(px_physics_->getTolerancesScale());
    default_scene_desc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    default_scene_desc.cpuDispatcher = px_cpu_dispatcher_;
    default_scene_desc.filterShader = physx::PxDefaultSimulationFilterShader;
    return px_physics_->createScene(default_scene_desc);
}

physx::PxScene* PhysicsMgr::load_px_scene(const char* scene_name)
{
    physx::PxScene* scene = create_px_scene();
    if (!scene) {
        spdlog::error("create px scene failed!");
        return nullptr;
    }

    UnityPhysicsLoader loader;
    if (loader.LoadFromFile(scene_name)) {
        spdlog::info("Data loaded successfully, creating PhysX scene...");

        if (loader.CreatePhysXScene(px_physics_, scene)) {
            spdlog::info("PhysX scene created successfully!");
        }
        else {
            spdlog::error("Failed to create PhysX scene from data");
        }
    }
    else {
        spdlog::error("Failed to load physics data for scene: {}, please make sure the file exists and is valid JSON", scene_name);
    }
    return scene;
}
