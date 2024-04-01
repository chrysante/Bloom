#include "Bloom/Graphics/RenderGraph/Pass.h"

#include <range/v3/algorithm.hpp>

#include "Bloom/Graphics/RenderGraph/ResourceNodes.h"

using namespace bloom;
using namespace rg;

Pass::~Pass() = default;

Resource* Pass::findInput(std::string_view name) {
    auto itr = ranges::find_if(_inputs, [&](auto& input) {
        return input->name() == name;
    });
    return itr != _inputs.end() ? itr->get() : nullptr;
}

Resource* Pass::findOutput(std::string_view name) {
    auto itr = ranges::find_if(_outputs, [&](auto& output) {
        return output->name() == name;
    });
    return itr != _outputs.end() ? itr->get() : nullptr;
}
