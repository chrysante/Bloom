#ifndef BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H
#define BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H

#include <vml/vml.hpp>

#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

enum class PhysicsComponentType { Static, Movable };

struct PhysicsStaticComponent {
    BLOOM_REGISTER_COMPONENT("PhysicsStaticComponent")
};

static_assert(std::is_empty_v<PhysicsStaticComponent>);

struct PhysicsMovableComponent {
    BLOOM_REGISTER_COMPONENT("PhysicsMovableComponent")
};

static_assert(std::is_empty_v<PhysicsMovableComponent>);

struct SphereColliderComponent {
    BLOOM_REGISTER_COMPONENT("SphereCollider")
    float radius = 100;
};

struct BoxColliderComponent {
    BLOOM_REGISTER_COMPONENT("BoxCollider")
    vml::packed_float3 halfExtend = { 50, 50, 50 };
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::PhysicsMovableComponent);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PhysicsStaticComponent);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SphereColliderComponent, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::BoxColliderComponent, halfExtend);

#endif // BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H
