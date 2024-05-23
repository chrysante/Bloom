#ifndef BLOOM_GRAPHICS_RENDERER2D_H
#define BLOOM_GRAPHICS_RENDERER2D_H

#include <span>
#include <vector>

#include <vml/vml.hpp>

#include "Bloom/GPU/CommandDescriptors.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Shader/Vertex.h"

namespace bloom {

class Renderer2DScene {
public:
    /// \Returns a view over the recorded vertices
    std::span<LineVertex2D const> vertices() const { return _vertices; };

    /// Draws a line from \p begin to \p end
    ///
    /// \Param thickness is in number of screen pixels
    void addLine(vml::float3 begin, vml::float3 end, vml::float4 color,
                 float thickness);

    ///
    void addCircle(vml::float3 position, vml::float3 normal, float radius,
                   vml::float4 color, float thickness, int numSegments = 0);

    /// Stateful API
    void beginLineStrip();
    void endLineStrip();
    void beginLineLoop();
    void endLineLoop();
    void addLinePoint(vml::float3 p, vml::float4 color, float thickness);

private:
    enum class State { None, LineStrip, LineLoop };

    struct Point {
        vml::float3 position;
        vml::float4 color;
        float thickness;
    };

    State state = State::None;
    std::optional<Point> lastPoint;
    std::optional<Point> loopBegin;
    std::vector<LineVertex2D> _vertices;
};

class Renderer2D {
public:
    /// Draws all recorded commands
    void drawInWorldCoords(Renderer2DScene const& scene, HardwareDevice& device,
                           CommandBuffer& commandBuffer,
                           BufferHandle cameraBuffer,
                           RenderPassDescription const& renderPassDesc);

private:
    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
    BufferHandle cameraBuffer;
    BufferHandle vertexBuffer;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER2D_H
