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
    explicit SceneRenderer(Renderer* renderer = nullptr) {
        mRenderer = renderer;
    }

    virtual ~SceneRenderer() = default;

    void setRenderer(Renderer* renderer) { mRenderer = renderer; }

    Renderer& renderer() const { return *mRenderer; }

    void draw(Scene const&, Camera const&, Framebuffer&, CommandQueue&);

    void draw(std::span<Scene const* const>, Camera const&, Framebuffer&,
              CommandQueue&);

private:
    virtual void submitScene(Scene const&);
    virtual void submitExtra() {}

    Renderer* mRenderer = nullptr;
};

} // namespace bloom
