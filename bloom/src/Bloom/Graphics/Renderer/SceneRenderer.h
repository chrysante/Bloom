#ifndef BLOOM_GRAPHICS_RENDERER_SCENERENDERER_H
#define BLOOM_GRAPHICS_RENDERER_SCENERENDERER_H

#include <span>

#include "Bloom/Core/Core.h"

namespace bloom {

class Renderer;
class Framebuffer;
class CommandQueue;
class Scene;
class Camera;

/// Wrapper around a `Renderer` to provide a simplified API for drawing a
/// `Scene`
class BLOOM_API SceneRenderer {
public:
    /// Construct from an underlying renderer
    explicit SceneRenderer(Renderer* renderer = nullptr) {
        mRenderer = renderer;
    }

    virtual ~SceneRenderer() = default;

    /// Update the underlying renderer
    void setRenderer(Renderer* renderer) { mRenderer = renderer; }

    /// \Returns the underlying renderer
    Renderer& renderer() const { return *mRenderer; }

    /// Submits the scenes \p scenes to the underlying renderer from the
    /// perspective of \p camera
    ///
    /// \Note This method does not draw the scenes. To draw the scenes call
    /// `draw()` on the underlying renderer
    void submitScenes(std::span<Scene const* const> scenes,
                      Camera const& camera);

private:
    ///
    virtual void submitScene(Scene const& scene);

    /// Called by `submitScenes` after submitting all scenes
    virtual void submitExtra() {}

    Renderer* mRenderer = nullptr;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_SCENERENDERER_H
