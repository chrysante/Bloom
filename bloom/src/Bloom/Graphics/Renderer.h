#ifndef BLOOM_GRAPHICS_RENDERER_H
#define BLOOM_GRAPHICS_RENDERER_H

#include <bit>
#include <cstring>
#include <span>
#include <vector>

#include <utl/utility.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Shader/SharedTypes.h"

namespace bloom {

class Scene;
class HardwareDevice;
class Camera;

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
class BLOOM_API Renderer {
public:
    static std::span<InterfaceUnion const> Sinks();

    static std::span<InterfaceUnion const> Sources();

    explicit Renderer(RenderGraph* renderGraph);

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
    GeometryResource* geometry = nullptr;
    BufferResource* cameraBuffer = nullptr;
    std::vector<vml::float4x4> lightSpaceTransforms;
    BufferResource* lightSpaceTransformBuffer = nullptr;
    ShadowRenderMetadata shadowMetadata;
    BufferResource* shadowMetadataBuffer;
    LightsResource* lights = nullptr;
    BufferResource* bloomParams = nullptr;
    BufferResource* postProcessParams = nullptr;
    FrameParameters frameParams;
    Camera camera;
    size_t sceneIndex = 0;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_H
