#include "Bloom/Graphics/Renderer/Renderer.h"

#include "Bloom/Application/Application.h"
#include "Bloom/Graphics/Renderer/ForwardRenderer.h"

namespace bloom {

std::unique_ptr<Renderer> createForwardRenderer(Application& application) {
    return std::make_unique<ForwardRenderer>(application.makeReceiver());
}

} // namespace bloom
