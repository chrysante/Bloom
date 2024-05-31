/// This file defines types that are analogously defined in the engine script
/// library

#ifndef BLOOM_RUNTIME_SCRIPTBINDINGS_H
#define BLOOM_RUNTIME_SCRIPTBINDINGS_H

#include <string>

#include <vml/vml.hpp>

#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Entity.h"

namespace bloom {

class Scene;

/// We replicate the transform component because we need to exactly match the
/// memory layout of the `Transform` type in script. This way we don't couple
/// the engine transform's memory with the script system
struct ScriptTransform {
    vml::packed_double3 position;
    vml::packed_double4 orientation; // Packed quaternions don't exist
    vml::packed_double3 scale;

    static ScriptTransform make(Transform const& t) {
        return { t.position, t.orientation, t.scale };
    }

    Transform native() const {
        return Transform{ position, orientation, scale };
    }

    operator Transform() const { return native(); }
};

/// See `ScriptTransform`
struct ScriptEntityHandle {
    EntityID::RawType ID;
    Scene* scene;

    static ScriptEntityHandle make(EntityHandle e) {
        return { e.ID().raw(), e.scene() };
    }

    EntityHandle native() const { return EntityHandle(EntityID(ID), scene); }

    operator EntityHandle() const { return native(); }
};

/// \Returns a string of Scatha source that defines global constants for each
/// key
std::string makeKeyTableSource();

} // namespace bloom

#endif // BLOOM_RUNTIME_SCRIPTBINDINGS_H
