#ifndef BLOOM_GRAPHICS_RENDERER_H
#define BLOOM_GRAPHICS_RENDERER_H

#include <bit>
#include <cstring>
#include <span>
#include <vector>

#include <utl/utility.hpp>

#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Shader/SharedTypes.h"

namespace bloom {

class Scene;
class HardwareDevice;
class Camera;

struct BloomSettings {
    bool physicallyCorrect;
    float intensity;
    float threshold;
    float knee;
    float clamp;
    float diffusion;
    float contribution;
};

struct PostProcessSettings {
    ToneMappingFunction toneMappingFunction;
};

/// Wrapper around a render graph that provides a simple scene submission
/// interface
class BLOOM_API Renderer {
public:
    static std::span<InterfaceUnion const> Sinks();

    static std::span<InterfaceUnion const> Sources();

    /// Constructs a renderer from a render graph. The lifetime of the renderer
    /// is tied to the render graph. If the render graph changes a new renderer
    /// must be constructed
    explicit Renderer(RenderGraph* renderGraph, CommandQueue& commandQueue);

    /// Resets the internal scene state and reallocates textures and other
    /// resource as necessary if \p frameParams changed since last frame
    void begin(FrameParameters const& frameParams, Camera const& camera,
               BloomSettings bloomSettings,
               PostProcessSettings postProcessSettings);

    /// Submits geometry and lights in \p scene to the corresponding resources
    void submit(Scene const& scene);

    /// Executes the render graph
    void draw(CommandBuffer& commandBuffer);

    ///
    FrameParameters const& getFrameParams() const { return frameParams; }

    ///
    Camera const& getCamera() const { return camera; }

    ///
    RenderGraph* renderGraph() const { return mRenderGraph; }

private:
    RenderGraph* mRenderGraph = nullptr;
    GeometryResource* geometry = nullptr;
    BufferResource* cameraBuffer = nullptr;
    TextureResource* specularTexture = nullptr;
    TextureResource* brdfTexture = nullptr;
    TextureHandle BRDF;
    std::vector<vml::float4x4> lightSpaceTransforms;
    BufferResource* lightSpaceTransformBuffer = nullptr;
    LightsResource* lights = nullptr;
    BufferResource* bloomParams = nullptr;
    BufferResource* postProcessParams = nullptr;
    FrameParameters frameParams;
    Camera camera;
    BloomSettings bloomSettings;
    PostProcessSettings postProcessSettings;
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::BloomSettings, physicallyCorrect, intensity,
                           threshold, knee, clamp, diffusion, contribution);

BLOOM_MAKE_TEXT_SERIALIZER(bloom::PostProcessSettings, toneMappingFunction);

#endif // BLOOM_GRAPHICS_RENDERER_H
