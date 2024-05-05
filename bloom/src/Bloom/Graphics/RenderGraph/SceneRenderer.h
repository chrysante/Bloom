#ifndef BLOOM_GRAPHICS_RENDERGRAPH_SCENERENDERER_H
#define BLOOM_GRAPHICS_RENDERGRAPH_SCENERENDERER_H

/// This file will replace Renderer/SceneRenderer once we get rid of the old
/// renderer class

#include <bit>
#include <cstring>
#include <vector>

#include <utl/utility.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
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

struct EntityHandleEncoding {
    EntityHandleEncoding() { std::memset(this, 0xff, sizeof *this); }

    EntityHandleEncoding(size_t sceneIdx, uint32_t rawID) {
        BL_ASSERT(rawID < (1 << 24));
        entityID = rawID;
        sceneIndex = utl::narrow_cast<uint8_t>(sceneIdx);
    }

    explicit EntityHandleEncoding(uint32_t raw) {
        std::memcpy(this, &raw, sizeof *this);
    }

    bool valid() const {
        EntityHandleEncoding ref;
        return std::memcmp(this, &ref, sizeof ref) != 0;
    }

    uint32_t raw() const { return std::bit_cast<uint32_t>(*this); }

    uint32_t entityID   : 24;
    uint32_t sceneIndex : 8;
};

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
    size_t sceneIndex = 0;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_SCENERENDERER_H
