#ifndef BLOOM_GRAPHICS_RENDERGRAPH_SCENERENDERER_H
#define BLOOM_GRAPHICS_RENDERGRAPH_SCENERENDERER_H

/// This file will replace Renderer/SceneRenderer once we get rid of the old
/// renderer class

#include <vector>

#include "Bloom/Core/Base.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Shader/SharedTypes.h"

namespace bloom {

class Scene;
class HardwareDevice;
class Camera;

} // namespace bloom

namespace bloom::rg {

class RenderGraph;
class GeometryResource;
class LightsResource;
class BufferResource;
struct FrameParameters;

///
class BLOOM_API SceneRenderer {
public:
    explicit SceneRenderer(RenderGraph* renderGraph);

    ///
    void begin(FrameParameters const& frameParams, Camera const& camera);

    ///
    void submit(Scene const& scene);

    ///
    void draw(CommandQueue& commandQueue);

    ///
    FrameParameters const& getFrameParams() const { return frameParams; }

    ///
    Camera const& getCamera() const { return camera; }

private:
    RenderGraph* mRenderGraph = nullptr;
    rg::GeometryResource* geometry = nullptr;
    rg::BufferResource* cameraBuffer = nullptr;
    std::vector<vml::float4x4> lightSpaceTransforms;
    rg::BufferResource* lightSpaceTransformBuffer = nullptr;
    ShadowRenderMetadata shadowMetadata;
    rg::BufferResource* shadowMetadataBuffer;
    rg::LightsResource* lights = nullptr;
    rg::BufferResource* bloomParams = nullptr;
    rg::BufferResource* postProcessParams = nullptr;
    FrameParameters frameParams;
    Camera camera;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_SCENERENDERER_H
