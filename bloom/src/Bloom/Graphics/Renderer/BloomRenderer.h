#ifndef BLOOM_GRAPHICS_RENDERER_BLOOMRENDERER_H
#define BLOOM_GRAPHICS_RENDERER_BLOOMRENDERER_H

#include <vml/vml.hpp>

#include "Bloom/Core/Core.h"
#include "Bloom/GPU/HardwarePrimitives.h"

#include "Bloom/Graphics/Renderer/BloomFramebuffer.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"

namespace bloom {

class HardwareDevice;
class CommandQueue;

struct BloomRenderParameters {
    bool enabled = true;
    bool physicallyCorrect = true;
    float intensity = 1;
    float threshold = 1;
    float knee = 0.1;
    float clamp = 100;
    float diffusion = 1;
    float contribution = 0.1;
};

class BLOOM_API BloomRenderer {
public:
    void init(HardwareDevice&);
    void populateFramebuffer(HardwareDevice&, BloomFramebuffer&,
                             vml::usize2) const;

    void render(CommandQueue&, BloomFramebuffer&, TextureView rawColor,
                BufferView renderParameters, vml::uint2 framebufferSize);

    BloomParameters makeShaderParameters() const;

    BloomRenderParameters getParameters() const { return settings; }
    void setParameters(BloomRenderParameters const&);

private:
    ComputePipelineHandle prefilterPipeline;
    ComputePipelineHandle downsamplePipeline;
    ComputePipelineHandle upsamplePipeline;

    BloomRenderParameters settings;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_BLOOMRENDERER_H
