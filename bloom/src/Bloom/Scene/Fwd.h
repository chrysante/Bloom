#ifndef BLOOM_SCENE_FWD_H
#define BLOOM_SCENE_FWD_H

#include <vml/vml.hpp>

namespace bloom {

class Scene;

struct PhysicsSimulationParameters {
    vml::float3 gravity = { 0, 0, -9.81 };
};

} // namespace bloom

#endif // BLOOM_SCENE_FWD_H
