#ifndef BLOOM_SCENE_COMPONENTS_SCRIPT_H
#define BLOOM_SCENE_COMPONENTS_SCRIPT_H

#include <scatha/Sema/Fwd.h>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

/// A script component is a heap allocated Scatha object
struct ScriptComponent {
    BLOOM_REGISTER_COMPONENT("Script")

    scatha::sema::StructType const* type = nullptr;
    scatha::sema::Function const* updateFunction = nullptr;
    uint64_t objectAddress = 0;
};

///
struct ScriptPreservedData {
    BLOOM_REGISTER_COMPONENT("Script Preserved Data")

    std::string typeName;
};

void BLOOM_API assignType(ScriptComponent& component,
                          scatha::sema::StructType const* type);

} // namespace bloom

template <>
struct YAML::convert<bloom::ScriptComponent> {
    static Node encode(bloom::ScriptComponent const&);
    static bool decode(Node const& node, bloom::ScriptComponent&);
};

template <>
struct YAML::convert<bloom::ScriptPreservedData> {
    static Node encode(bloom::ScriptPreservedData const&) { return Node(); }
    static bool decode(Node const& node, bloom::ScriptPreservedData&) {
        return true;
    }
};

#endif // BLOOM_SCENE_COMPONENTS_SCRIPT_H
