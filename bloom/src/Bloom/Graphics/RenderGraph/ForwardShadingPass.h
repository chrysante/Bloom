#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Node.h"
#include "Bloom/Graphics/RenderGraph/Scene.h"

namespace bloom::rg {

///
class ForwardShadingPass: public Pass {
public:
    explicit ForwardShadingPass(std::string name);

private:
    void doExecute(RenderContext& ctx) override;

    // Inputs
    BufferHandle sceneParameters;
    SceneGeometry* geometry;
    SceneLights* lights;

    // Outputs
    TextureHandle rawColor;
    TextureHandle depth;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H
