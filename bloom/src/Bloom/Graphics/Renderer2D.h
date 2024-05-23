#ifndef BLOOM_GRAPHICS_RENDERER2D_H
#define BLOOM_GRAPHICS_RENDERER2D_H

#include <vector>

#include <utl/structure_of_arrays.hpp>
#include <vml/vml.hpp>

#include "Bloom/GPU/CommandDescriptors.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Shader/Vertex.h"

namespace bloom {

UTL_SOA_TYPE(Renderer2DScene, (size_t, vertexBegin), (size_t, vertexCount));

class Renderer2D {
public:
    void begin();

    void beginScene();

    void endScene();

    /// Draws a line from \p begin to \p end
    ///
    /// \Param thickness is in screen pixel
    void line(vml::float3 begin, vml::float3 end, vml::float4 color,
              float thickness);

    /// Draws all recorded commands
    void draw(BufferHandle cameraBuffer, HardwareDevice& device,
              CommandBuffer& commandBuffer,
              RenderPassDescription const& renderPassDesc);

private:
    Renderer2DScene::reference current();

    utl::structure_of_arrays<Renderer2DScene> scenes;
    std::vector<LineVertex2D> vertices;

    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
    BufferHandle cameraBuffer;
    BufferHandle vertexBuffer;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER2D_H
