#ifndef BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H
#define BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H

#include <vml/vml.hpp>

#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct PhysicsComponent {
    BLOOM_REGISTER_COMPONENT("PhysicsComponent")
    enum Kind { Static, Movable };
    Kind kind = Kind::Static;
};

struct SphereColliderComponent {
    BLOOM_REGISTER_COMPONENT("SphereCollider")
    float radius = 100;
};

struct BoxColliderComponent {
    BLOOM_REGISTER_COMPONENT("BoxCollider")
    vml::packed_float3 halfExtend = { 50, 50, 50 };
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::PhysicsComponent, kind);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SphereColliderComponent, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::BoxColliderComponent, halfExtend);

#endif // BLOOM_SCENE_COMPONENTS_PHYSICSCOMPONENTS_H
