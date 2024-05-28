#include "Bloom/Runtime/PhysicsSystem.h"

#include <cstdarg>
#include <thread>

// <Jolt/Jolt.h> must be included first but clang-format sorts the includes
// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
// clang-format on
#include <magic_enum.hpp>

#include "Bloom/Core/Debug.h"

using namespace bloom;

struct PhysicsWorld::Impl {
    void init();
};

PhysicsWorld::PhysicsWorld(): impl(std::make_unique<PhysicsWorld::Impl>()) {}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::init() { impl->init(); }

void PhysicsWorld::Impl::init() {}

static void traceCallback(char const* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);
    Logger::Trace("Physics: ", buffer);
}

[[maybe_unused]]
static bool assertionCallback(char const* expr, char const* msg,
                              char const* file, uint line) {
    Logger::Error("Physics assertion failed: ", expr, " \"", msg, "\" in file ",
                  file, " on line ", line);
    return true;
};

namespace {

enum class Layer : JPH::ObjectLayer { Static, Movable };

struct ObjectLayerPairFilterImpl: public JPH::ObjectLayerPairFilter {
    bool ShouldCollide(JPH::ObjectLayer object1,
                       JPH::ObjectLayer object2) const override {
        switch (Layer{ object1 }) {
        case Layer::Static:
            return Layer{ object2 } ==
                   Layer::Movable; // Non moving only collides with moving
        case Layer::Movable:
            return true; // Moving collides with everything
        default:
            BL_UNREACHABLE();
        }
    }
};

namespace BroadPhaseLayers {

static constexpr JPH::BroadPhaseLayer Static(0);
static constexpr JPH::BroadPhaseLayer Movable(1);
static constexpr JPH::BroadPhaseLayer COUNT(2);

}; // namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
struct BPLayerInterfaceImpl final: public JPH::BroadPhaseLayerInterface {
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        objectToBroadPhase[(size_t)Layer::Static] = BroadPhaseLayers::Static;
        objectToBroadPhase[(size_t)Layer::Movable] = BroadPhaseLayers::Movable;
    }

    uint GetNumBroadPhaseLayers() const override {
        return (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::COUNT;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(
        JPH::ObjectLayer layer) const override {
        BL_ASSERT(layer < magic_enum::enum_count<Layer>());
        return objectToBroadPhase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    char const* GetBroadPhaseLayerName(
        JPH::BroadPhaseLayer layer) const override {
        switch ((JPH::BroadPhaseLayer::Type)layer) {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Static:
            return "Static";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Movable:
            return "Movable";
        default:
            BL_UNREACHABLE();
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    JPH::BroadPhaseLayer objectToBroadPhase[magic_enum::enum_count<Layer>()];
};

/// Class that determines if an object layer can collide with a broadphase layer
struct ObjectVsBroadPhaseLayerFilterImpl:
    public JPH::ObjectVsBroadPhaseLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer layer1,
                       JPH::BroadPhaseLayer layer2) const override {
        switch (Layer{ layer1 }) {
        case Layer::Static:
            return layer2 == BroadPhaseLayers::Movable;
        case Layer::Movable:
            return true;
        default:
            BL_UNREACHABLE();
        }
    }
};

struct MyContactListener: public JPH::ContactListener {
    // See: ContactListener
    JPH::ValidateResult OnContactValidate(
        JPH::Body const& body1, JPH::Body const& body2,
        JPH::RVec3Arg baseOffset,
        JPH::CollideShapeResult const& collisionResult) override {
        (void)body1;
        (void)body2;
        (void)baseOffset;
        (void)collisionResult;
        Logger::Info("Contact validate callback");
        // Allows you to ignore a contact before it is created (using layers to
        // not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(JPH::Body const& body1, JPH::Body const& body2,
                        JPH::ContactManifold const& manifold,
                        JPH::ContactSettings& ioSettings) override {
        (void)body1;
        (void)body2;
        (void)manifold;
        (void)ioSettings;
        Logger::Info("A contact was added");
    }

    void OnContactPersisted(JPH::Body const& body1, JPH::Body const& body2,
                            JPH::ContactManifold const& manifold,
                            JPH::ContactSettings& ioSettings) override {
        (void)body1;
        (void)body2;
        (void)manifold;
        (void)ioSettings;
        Logger::Info("A contact was persisted");
    }

    void OnContactRemoved(JPH::SubShapeIDPair const& subShapePair) override {
        (void)subShapePair;
        Logger::Info("A contact was removed");
    }
};

// An example activation listener
struct MyBodyActivationListener: public JPH::BodyActivationListener {
    void OnBodyActivated(JPH::BodyID const& bodyID,
                         uint64_t bodyUserData) override {
        (void)bodyID;
        (void)bodyUserData;
        Logger::Trace("A body got activated");
    }

    void OnBodyDeactivated(JPH::BodyID const& bodyID,
                           uint64_t bodyUserData) override {
        (void)bodyID;
        (void)bodyUserData;
        Logger::Trace("A body went to sleep");
    }
};

} // namespace

struct PhysicsSystem::Impl {
    void init();

    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    MyBodyActivationListener body_activation_listener;
    MyContactListener contact_listener;
    JPH::PhysicsSystem system;
};

PhysicsSystem::PhysicsSystem(): impl(std::make_unique<PhysicsSystem::Impl>()) {}

PhysicsSystem::~PhysicsSystem() = default;

void PhysicsSystem::init() { impl->init(); }

void PhysicsSystem::Impl::init() {
    JPH::RegisterDefaultAllocator();
    JPH::Trace = traceCallback;
    //    JPH::AssertFailed = assertionCallback;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    JPH::TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
    JPH::JobSystemThreadPool job_system(/*cMaxPhysicsJobs*/ 8,
                                        /*cMaxPhysicsBarriers*/ 8,
                                        std::thread::hardware_concurrency() -
                                            1);
    uint const MaxBodies = 65536;
    uint const NumBodyMutexes = 0;
    uint const MaxBodyPairs = 65536;
    uint const MaxContactConstraints = 10240;
    system.Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints,
                broad_phase_layer_interface, object_vs_broadphase_layer_filter,
                object_vs_object_layer_filter);

    system.SetBodyActivationListener(&body_activation_listener);
    system.SetContactListener(&contact_listener);
    JPH::BodyInterface& body_interface = system.GetBodyInterface();

    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
    JPH::ShapeSettings::ShapeResult floor_shape_result =
        floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape =
        floor_shape_result
            .Get(); // We don't expect an error here, but you can check
                    // floor_shape_result for HasError() / GetError()
    JPH::BodyCreationSettings floor_settings(floor_shape,
                                             JPH::RVec3(0.0, -1.0, 0.0),
                                             JPH::Quat::sIdentity(),
                                             JPH::EMotionType::Static,
                                             (uint32_t)Layer::Static);
    JPH::Body* floor = body_interface.CreateBody(
        floor_settings); // Note that if we run out of bodies this can return
                         // nullptr
    body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f),
                                              JPH::RVec3(0.0, 2.0, 0.0),
                                              JPH::Quat::sIdentity(),
                                              JPH::EMotionType::Dynamic,
                                              (uint32_t)Layer::Movable);
    JPH::BodyID sphere_id =
        body_interface.CreateAndAddBody(sphere_settings,
                                        JPH::EActivation::Activate);
    body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -5.0f, 0.0f));
    system.OptimizeBroadPhase();
}
