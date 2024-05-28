#ifndef BLOOM_RUNTIME_PHYSICSSYSTEM_H
#define BLOOM_RUNTIME_PHYSICSSYSTEM_H

#include <memory>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Scene/Fwd.h"

namespace bloom {

class BLOOM_API PhysicsSystem: public CoreSystem {
public:
    PhysicsSystem();

    ~PhysicsSystem();

    void init();

    void onSceneInit(Scene& scene, PhysicsSimulationParameters parameters);

    void onSceneStop();

    void update(Scene& scene, Timestep time);

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_PHYSICSSYSTEM_H
