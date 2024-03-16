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

YAML::Node YAML::convert<bloom::ScriptComponent>::encode(
    bloom::ScriptComponent const& s) {
    Node root;
    return root;
}

bool YAML::convert<bloom::ScriptComponent>::decode(Node const& root,
                                                   bloom::ScriptComponent& s) {
    return true;
}
