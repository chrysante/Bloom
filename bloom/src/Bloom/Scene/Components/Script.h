#ifndef BLOOM_SCENE_COMPONENTS_SCRIPT_H
#define BLOOM_SCENE_COMPONENTS_SCRIPT_H

// #include <scatha/Sema/Fwd.h>

#include "Bloom/Core/Serialize.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct ScriptComponent {
    BLOOM_REGISTER_COMPONENT("Script")

    // scatha::sema::StructType const* classType = nullptr;
    void* object = nullptr;
};

} // namespace bloom

template <>
struct YAML::convert<bloom::ScriptComponent> {
    static Node encode(bloom::ScriptComponent const&);
    static bool decode(Node const& node, bloom::ScriptComponent&);
};

#endif // BLOOM_SCENE_COMPONENTS_SCRIPT_H
