#include "Bloom/Graphics/Renderer/Renderer.hpp"

#include "Bloom/Application/Application.hpp"
#include "Bloom/Graphics/Renderer/ForwardRenderer.hpp"

namespace bloom {

std::unique_ptr<Renderer> createForwardRenderer(Application& application) {
    return std::make_unique<ForwardRenderer>(application.makeReciever());
}

} // namespace bloom
