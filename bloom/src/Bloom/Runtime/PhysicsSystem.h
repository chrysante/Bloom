#ifndef BLOOM_RUNTIME_PHYSICSSYSTEM_H
#define BLOOM_RUNTIME_PHYSICSSYSTEM_H

#include <memory>

#include "Bloom/Core/Base.h"

namespace bloom {

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

class BLOOM_API PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void init();

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_PHYSICSSYSTEM_H
