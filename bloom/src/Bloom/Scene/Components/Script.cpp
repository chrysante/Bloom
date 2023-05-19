#include "Bloom/Scene/Components/Script.hpp"

#include "Bloom/Application/Application.hpp"

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
