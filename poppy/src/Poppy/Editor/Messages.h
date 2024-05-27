#ifndef POPPY_EDITOR_MESSAGES_H
#define POPPY_EDITOR_MESSAGES_H

#include <any>

namespace bloom {

class RenderGraph;

}

namespace poppy {

///
struct ExtendRenderLifetime {
    std::any object;
};

/// Sent by the render graph editor to notify renderers using the graph to
/// update
struct DidRecompileRenderGraph {
    bloom::RenderGraph const* renderGraph = nullptr;
};

} // namespace poppy

#endif // POPPY_EDITOR_MESSAGES_H
