#pragma once

#include <optional>

#include <mtl/mtl.hpp>

#include "Bloom/Core/Serialize.hpp"
#include "Bloom/Scene/Components/ComponentBase.hpp"

namespace bloom {

struct ScriptComponent {
    BLOOM_REGISTER_COMPONENT("Script");

    int dummy;
};

} // namespace bloom

template <>
struct YAML::convert<bloom::ScriptComponent> {
    static Node encode(bloom::ScriptComponent const&);
    static bool decode(Node const& node, bloom::ScriptComponent&);
};
