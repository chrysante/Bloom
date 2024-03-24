/// This file defines types that are analogously defined in the engine script library

#ifndef BLOOM_RUNTIME_SCRIPTBINDINGS_H
#define BLOOM_RUNTIME_SCRIPTBINDINGS_H

#include <mtl/mtl.hpp>

#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Entity.h"

namespace bloom {

class Scene;

struct ScriptTransform {
    mtl::packed_double3 position;
    mtl::packed_double4 orientation; // Packed quaternions don't exist
    mtl::packed_double3 scale;

    static ScriptTransform make(Transform const& t) {
        return { t.position, t.orientation, t.scale };
    }

    explicit operator Transform() const {
        return Transform{ position,
                          mtl::quaternion_float{
                              orientation.x,
                              orientation.y,
                              orientation.z,
                              orientation.w,
                          },
                          scale };
    }
};

struct ScriptEntityHandle {
    EntityID::RawType ID;
    Scene* scene;

    static ScriptEntityHandle make(EntityHandle e) {
        return { e.ID().raw(), &e.scene() };
    }

    explicit operator EntityHandle() const {
        return EntityHandle(EntityID(ID), scene);
    }
};

} // namespace bloom

#endif // BLOOM_RUNTIME_SCRIPTBINDINGS_H
