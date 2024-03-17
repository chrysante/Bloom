#ifndef BLOOM_SCENE_COMPONENTS_SCRIPT_H
#define BLOOM_SCENE_COMPONENTS_SCRIPT_H

#include <scatha/Sema/Fwd.h>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

/// A script component is a heap allocated Scatha object.
/// Can be used to add arbrtrary runtime defined behaviour to an entity
struct ScriptComponent {
    BLOOM_REGISTER_COMPONENT("Script")

    /// The class type of the object that will be instantiated for the entity
    scatha::sema::StructType const* type = nullptr;

    /// The update function of the class
    scatha::sema::Function const* updateFunction = nullptr;

    /// The address of the allocated object
    uint64_t objectAddress = 0;
};

/// Data that can be serialized. Also used to preserve object data through
/// recompilations
struct ScriptPreservedData {
    BLOOM_REGISTER_COMPONENT("Script Preserved Data")

    /// The name of the class type
    std::string classname;
};

void BLOOM_API assignType(ScriptComponent& component,
                          scatha::sema::StructType const* type);

} // namespace bloom

template <>
struct YAML::convert<bloom::ScriptComponent> {
    static Node encode(bloom::ScriptComponent const&) { return Node(); }
    static bool decode(Node const&, bloom::ScriptComponent&) { return true; }
};

template <>
struct YAML::convert<bloom::ScriptPreservedData> {
    static Node encode(bloom::ScriptPreservedData const&);
    static bool decode(Node const& node, bloom::ScriptPreservedData&);
};

#endif // BLOOM_SCENE_COMPONENTS_SCRIPT_H
