#include "Bloom/Graphics/RenderGraph/Node.h"

#include <range/v3/algorithm.hpp>

#include "Bloom/Graphics/RenderGraph/Scene.h"

using namespace bloom;
using namespace rg;

Resource::~Resource() = default;

Resource::Resource(std::string name, ResourceHandleVariant handle):
    _name(std::move(name)), _handle(std::move(handle)) {}

void Resource::addUser(PassInput* user) {
    if (!ranges::contains(_users, user)) {
        _users.push_back(user);
    }
}

PassInput::PassInput(InterfaceVariant interface):
    _interface(std::move(interface)) {}

PassOutput::PassOutput(InterfaceVariant interface):
    Resource(interface.name(), {}), _interface(std::move(interface)) {}

UserResource::UserResource(std::string name, ResourceHandleVariant handle):
    Resource(std::move(name), std::move(handle)) {}

PassInput* Pass::findInput(std::string_view name) {
    auto itr = ranges::find_if(_inputs, [&](auto& input) {
        return input->name() == name;
    });
    return itr != _inputs.end() ? itr->get() : nullptr;
}

PassOutput* Pass::findOutput(std::string_view name) {
    auto itr = ranges::find_if(_outputs, [&](auto& output) {
        return output->name() == name;
    });
    return itr != _outputs.end() ? itr->get() : nullptr;
}

void Pass::declareInput(BufferInterface const& interface) {
    _inputs.push_back(std::make_unique<PassInput>(interface));
}

void Pass::declareInput(TextureInterface const& interface) {
    _inputs.push_back(std::make_unique<PassInput>(interface));
}

void Pass::declareInput(SceneGeometryInterface const& interface) {
    _inputs.push_back(std::make_unique<PassInput>(interface));
}

void Pass::declareInput(SceneLightsInterface const& interface) {
    _inputs.push_back(std::make_unique<PassInput>(interface));
}

void Pass::declareOutput(BufferInterface const& interface) {
    _outputs.push_back(std::make_unique<PassOutput>(interface));
}

void Pass::declareOutput(TextureInterface const& interface) {
    _outputs.push_back(std::make_unique<PassOutput>(interface));
}
