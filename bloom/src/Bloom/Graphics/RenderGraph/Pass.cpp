#include "Bloom/Graphics/RenderGraph/Pass.h"

#include <range/v3/algorithm.hpp>

#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"

using namespace bloom;
using namespace rg;

Pass::~Pass() = default;

PassSink* Pass::findSink(std::string_view name) {
    auto itr = ranges::find_if(_sinks, [&](auto& sink) {
        return sink->name() == name;
    });
    return itr != _sinks.end() ? itr->get() : nullptr;
}

PassSource* Pass::findSource(std::string_view name) {
    auto itr = ranges::find_if(_sources, [&](auto& source) {
        return source->name() == name;
    });
    return itr != _sources.end() ? itr->get() : nullptr;
}

void Pass::execute(FrameParameters params, CommandQueue& queue) {
    _frameParams = params;
    if (auto* renderPass = dynamic_cast<RenderPass*>(this)) {
        renderPass->doExecute(*queue.createRenderContext());
    }
    else if (auto* computePass = dynamic_cast<ComputePass*>(this)) {
        computePass->doExecute(*queue.createComputeContext());
    }
    else {
        BL_UNIMPLEMENTED();
    }
}
