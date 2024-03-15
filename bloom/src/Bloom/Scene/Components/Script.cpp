#include "Bloom/Scene/Components/Script.h"

#include "Bloom/Application/Application.h"

using namespace bloom;

YAML::Node YAML::convert<bloom::ScriptComponent>::encode(
    bloom::ScriptComponent const& s) {
    Node root;
    return root;
}

bool YAML::convert<bloom::ScriptComponent>::decode(Node const& root,
                                                   bloom::ScriptComponent& s) {
    return true;
}
