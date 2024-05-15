#ifndef POPPY_EDITOR_MESSAGES_H
#define POPPY_EDITOR_MESSAGES_H

#include <any>

namespace poppy {

/// Instructs the render graph editor to compile the render graph
struct CompileRenderGraph {};

///
struct ExtendRenderLifetime {
    std::any object;
};

} // namespace poppy

#endif // POPPY_EDITOR_MESSAGES_H
