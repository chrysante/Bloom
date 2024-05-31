#ifndef BLOOM_RUNTIME_PHYSICSSYSTEM_H
#define BLOOM_RUNTIME_PHYSICSSYSTEM_H

#include <memory>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Scene/Components/PhysicsComponents.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Fwd.h"

namespace bloom {

/// Wrapper class around the external physics system (JPH)
class BLOOM_API PhysicsSystem: public CoreSystem {
public:
    struct Settings {
        int collisionStepsPerFrame = 1;
    };

    PhysicsSystem();

    ~PhysicsSystem();

    void init();

    void onSceneInit(Scene& scene, PhysicsBehaviourParameters parameters);

    void onSceneStop();

    void update(Scene& scene, Timestep time);

    void setTransform(uint32_t bodyID, Transform const& transform);

    Settings settings() const;

    void setSettings(Settings settings);

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_PHYSICSSYSTEM_H
