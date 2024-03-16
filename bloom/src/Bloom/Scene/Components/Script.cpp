#include "Bloom/Scene/Components/Script.h"

#include <scatha/sema/Entity.h>

#include "Bloom/Application/Application.h"

using namespace bloom;

void bloom::assignType(ScriptComponent& component,
                       scatha::sema::StructType const* type) {
    if (!type) {
        return;
    }
    component.type = type;
    auto functions = type->findFunctions("update");
    component.updateFunction = !functions.empty() ? functions.front() : nullptr;
}

YAML::Node YAML::convert<bloom::ScriptPreservedData>::encode(
    bloom::ScriptPreservedData const& data) {
    Node node;
    node["Typename"] = data.classname;
    return node;
}

bool YAML::convert<bloom::ScriptPreservedData>::decode(
    Node const& node, bloom::ScriptPreservedData& data) {
    data.classname = node["Typename"].as<std::string>();
    return true;
}
