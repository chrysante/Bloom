#ifndef BLOOM_RUNTIME_PHYSICSSYSTEM_H
#define BLOOM_RUNTIME_PHYSICSSYSTEM_H

#include <memory>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Time.h"

namespace bloom {

class Scene;

class BLOOM_API PhysicsBody {
public:
private:
};

class BLOOM_API PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void init();

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
};

class BLOOM_API PhysicsSystem: public CoreSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void init();

    void onSceneInit(Scene& scene);

    void update(Scene& scene, Timestep time);

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_PHYSICSSYSTEM_H
