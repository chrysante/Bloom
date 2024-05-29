#ifndef BLOOM_SCENE_FWD_H
#define BLOOM_SCENE_FWD_H

#include <vml/vml.hpp>

#include "Bloom/Core/Yaml.h"

namespace bloom {

class Scene;

struct PhysicsBehaviourParameters {
    vml::float3 gravity = { 0, 0, -9.81 };
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::PhysicsBehaviourParameters, gravity);

#endif // BLOOM_SCENE_FWD_H
