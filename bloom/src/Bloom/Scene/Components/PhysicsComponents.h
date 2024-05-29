#ifndef BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H
#define BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H

#include <cstdint>

#include <vml/vml.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

enum class PhysicsComponentType { Static, Movable };

/// Tag component that makes the entity part of the static physics layer
struct BLOOM_API PhysicsStaticComponent {
    /// Will be set by the physics system when simulation starts
    uint32_t bodyID;
};

/// Tag component that makes the entity part of the dynamic physics layer
struct BLOOM_API PhysicsMovableComponent {
    /// Will be set by the physics system when simulation starts
    uint32_t bodyID;
};

/// Different colliders. Entities need a collider in order to have physics
/// simulated

/// Sphere centered at the origin of the entities transform
struct BLOOM_API SphereColliderComponent {
    float radius = 100;
};

/// Box centered at the origin of the entities transform
struct BLOOM_API BoxColliderComponent {
    vml::packed_float3 halfExtend = { 50, 50, 50 };
};

} // namespace bloom

/// We are not serializing the body IDs because they are only valid during
/// simulation and get assigned when the simulation starts
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PhysicsMovableComponent);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PhysicsStaticComponent);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SphereColliderComponent, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::BoxColliderComponent, halfExtend);

#endif // BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H
