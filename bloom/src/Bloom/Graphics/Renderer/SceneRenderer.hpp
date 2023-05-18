#pragma once

#include <span>

#include "Bloom/Core/Core.hpp"

namespace bloom {

class Renderer;
class Framebuffer;
class CommandQueue;
class Scene;
class Camera;

class BLOOM_API SceneRenderer {
public:
    SceneRenderer() = default;

    SceneRenderer(Renderer&);

    void setRenderer(Renderer& renderer);

    Renderer& renderer() const { return *mRenderer; }

    void draw(Scene const&, Camera const&, Framebuffer&, CommandQueue&);

    void draw(std::span<Scene const* const>,
              Camera const&,
              Framebuffer&,
              CommandQueue&);

protected:
    /// Overridable
    virtual void submitScene(Scene const&);
    virtual void submitExtra() {}

private:
    Renderer* mRenderer = nullptr;
};

} // namespace bloom
