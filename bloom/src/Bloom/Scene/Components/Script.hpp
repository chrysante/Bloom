#pragma once

// #include <scatha/Sema/Fwd.h>

#include "Bloom/Core/Serialize.hpp"
#include "Bloom/Scene/Components/ComponentBase.hpp"

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
