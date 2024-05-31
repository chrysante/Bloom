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
#include <range/v3/view.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Scene/Scene.h"

using namespace bloom;
using namespace vml::short_types;

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
    void onSceneInit(Scene& scene, PhysicsBehaviourParameters parameters);
    void onSceneStop();
    void update(Scene& scene, Timestep time);

    Settings settings{};
    std::atomic<bool> isSimulating = false;
    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    MyBodyActivationListener body_activation_listener;
    MyContactListener contact_listener;
    std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator;
    std::unique_ptr<JPH::JobSystemThreadPool> job_system;
    std::unique_ptr<JPH::PhysicsSystem> system;
};

PhysicsSystem::PhysicsSystem(): impl(std::make_unique<PhysicsSystem::Impl>()) {}

PhysicsSystem::~PhysicsSystem() = default;

void PhysicsSystem::init() { impl->init(); }
void PhysicsSystem::onSceneInit(Scene& scene,
                                PhysicsBehaviourParameters parameters) {
    impl->onSceneInit(scene, parameters);
}
void PhysicsSystem::onSceneStop() { impl->onSceneStop(); }
void PhysicsSystem::update(Scene& scene, Timestep time) {
    impl->update(scene, time);
}

void PhysicsSystem::Impl::init() {
    JPH::RegisterDefaultAllocator();
    JPH::Trace = traceCallback;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    job_system = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        (int)(std::thread::hardware_concurrency() - 1));
}

static float const ScaleRatio = 100;

static JPH::Vec3 toJPH(float3 v) { return JPH::RVec3(v.x, v.y, v.z); }

static JPH::Vec3 toJPHScaled(float3 v) {
    return JPH::RVec3(v.x, v.y, v.z) / ScaleRatio;
}

static JPH::Quat toJPH(quaternion_float q) {
    return JPH::Quat(q.imag.x, q.imag.y, q.imag.z, q.real);
}

[[maybe_unused]] static float3 toVML(JPH::Vec3 v) {
    return float3(v.GetX(), v.GetY(), v.GetZ());
}

static float3 toVMLScaled(JPH::Vec3 v) {
    return ScaleRatio * float3(v.GetX(), v.GetY(), v.GetZ());
}

static quaternion_float toVML(JPH::Quat v) {
    return quaternion_float(v.GetW(), v.GetX(), v.GetY(), v.GetZ());
}

static JPH::BodyID addObject(JPH::BodyInterface& interface,
                             Transform const& transform,
                             SphereColliderComponent collider, Layer layer,
                             JPH::EMotionType motionType,
                             JPH::EActivation activation) {
    float radius = collider.radius * transform.scale.fold(ranges::max);
    JPH::BodyCreationSettings settings(new JPH::SphereShape(radius /
                                                            ScaleRatio),
                                       toJPHScaled(transform.position),
                                       toJPH(transform.orientation), motionType,
                                       (uint32_t)layer);
    return interface.CreateAndAddBody(settings, activation);
}

/// We adjust the half extend of box colliders to be always greater than the
/// convex radius. Otherwise shape creation fails
static JPH::Vec3 adjustExtendToConvexRadius(JPH::Vec3 halfExtend,
                                            float radius) {
    for (int i = 0; i < 3; ++i) {
        halfExtend.SetComponent(i, std::max(halfExtend[i], radius + 0.01f));
    }
    return halfExtend;
}

static JPH::BodyID addObject(JPH::BodyInterface& interface,
                             Transform const& transform,
                             BoxColliderComponent collider, Layer layer,
                             JPH::EMotionType motionType,
                             JPH::EActivation activation) {
    JPH::Vec3 halfExtend =
        adjustExtendToConvexRadius(toJPHScaled(collider.halfExtend *
                                               transform.scale),
                                   JPH::cDefaultConvexRadius);
    JPH::BoxShapeSettings shapeSettings(halfExtend, JPH::cDefaultConvexRadius);
    JPH::ShapeSettings::ShapeResult shape = shapeSettings.Create();
    // We don't expect an error here, but you can check floor_shape_result for
    // HasError() / GetError()
    BL_ASSERT(!shape.HasError(), shape.GetError().c_str());
    JPH::BodyCreationSettings settings(shape.Get(),
                                       toJPHScaled(transform.position),
                                       toJPH(transform.orientation), motionType,
                                       (uint32_t)layer);
    // Note that if we run out of bodies this can return nullptr
    return interface.CreateAndAddBody(settings, activation);
}

template <typename PhysicsComponent, typename... Colliders>
static void addObjects(JPH::BodyInterface& interface, Scene& scene, Layer layer,
                       JPH::EMotionType motionType,
                       JPH::EActivation activation) {
    auto impl = [&]<typename Collider> {
        auto view = scene.view<PhysicsComponent, Collider const>();
        view.each(
            [&](EntityID ID, PhysicsComponent& phys, Collider const& collider) {
            auto bodyID = addObject(interface,
                                    scene.calculateWorldTransform(ID), collider,
                                    layer, motionType, activation);
            phys.bodyID = bodyID.GetIndexAndSequenceNumber();
        });
    };
    (impl.template operator()<Colliders>(), ...);
}

void PhysicsSystem::Impl::onSceneInit(Scene& scene,
                                      PhysicsBehaviourParameters parameters) {
    uint const MaxBodies = 65536;
    uint const NumBodyMutexes = 0;
    uint const MaxBodyPairs = 65536;
    uint const MaxContactConstraints = 10240;
    system = std::make_unique<JPH::PhysicsSystem>();
    system->Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints,
                 broad_phase_layer_interface, object_vs_broadphase_layer_filter,
                 object_vs_object_layer_filter);
    JPH::BodyInterface& interface = system->GetBodyInterface();
    addObjects<PhysicsStaticComponent, SphereColliderComponent,
               BoxColliderComponent>(interface, scene, Layer::Static,
                                     JPH::EMotionType::Static,
                                     JPH::EActivation::DontActivate);
    addObjects<PhysicsMovableComponent, SphereColliderComponent,
               BoxColliderComponent>(interface, scene, Layer::Movable,
                                     JPH::EMotionType::Dynamic,
                                     JPH::EActivation::Activate);
    system->OptimizeBroadPhase();
    system->SetGravity(toJPH(parameters.gravity));
    isSimulating = true;
}

void PhysicsSystem::Impl::onSceneStop() {
    system.reset();
    isSimulating = false;
}

void PhysicsSystem::Impl::update(Scene& scene, Timestep time) {
    system->Update(time.delta, settings.collisionStepsPerFrame,
                   temp_allocator.get(), job_system.get());
    auto& interface = system->GetBodyInterface();
    auto view = scene.view<PhysicsMovableComponent const, Transform>();
    view.each([&](PhysicsMovableComponent phys, Transform& transform) {
        JPH::RVec3 position;
        JPH::Quat rotation;
        interface.GetPositionAndRotation(JPH::BodyID(phys.bodyID), position,
                                         rotation);
        transform.position = toVMLScaled(position);
        transform.orientation = toVML(rotation);
    });
}

void PhysicsSystem::setTransform(uint32_t bodyID, Transform const& transform) {
    auto& interface = impl->system->GetBodyInterface();
    interface.SetPositionAndRotation(JPH::BodyID(bodyID),
                                     toJPHScaled(transform.position),
                                     toJPH(transform.orientation),
                                     JPH::EActivation::Activate);
}

PhysicsSystem::Settings PhysicsSystem::settings() const {
    return impl->settings;
}

void PhysicsSystem::setSettings(Settings settings) {
    if (impl->isSimulating) {
        return;
    }
    impl->settings = settings;
}
