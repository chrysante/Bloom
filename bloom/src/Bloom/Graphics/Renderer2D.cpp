#include "Bloom/Graphics/Renderer2D.h"

#include <range/v3/view.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

void Renderer2DScene::addLine(float3 begin, float3 end, float4 color,
                              float thickness) {
    beginLineStrip();
    addLinePoint(begin, color, thickness);
    addLinePoint(end, color, thickness);
    endLineStrip();
}

void Renderer2DScene::addCircle(vml::float3 position, vml::float3 normal,
                                float radius, vml::float4 color,
                                float thickness, int numSegments) {
    normal = vml::normalize(normal);
    if (numSegments == 0) {
        numSegments = 32;
    }
    float3 binormal = normal.x == 1.0 ? float3(0, 1, 0) :
                                        float3(normal.x, -normal.z, normal.y);
    quaternion_float q =
        vml::make_rotation(2 * vml::constants<>::pi / numSegments, normal);
    beginLineLoop();
    for (int i = 0; i < numSegments; ++i, binormal = vml::rotate(binormal, q)) {
        addLinePoint(position + radius * binormal, color, thickness);
    }
    endLineLoop();
}

void Renderer2DScene::beginLineStrip() {
    BL_ASSERT(state == State::None);
    state = State::LineStrip;
}

void Renderer2DScene::endLineStrip() {
    BL_ASSERT(state == State::LineStrip);
    lastPoint = std::nullopt;
    state = State::None;
}

void Renderer2DScene::beginLineLoop() {
    BL_ASSERT(state == State::None);
    state = State::LineLoop;
}

void Renderer2DScene::endLineLoop() {
    BL_ASSERT(state == State::LineLoop);
    if (loopBegin) {
        addLinePoint(loopBegin->position, loopBegin->color,
                     loopBegin->thickness);
    }
    loopBegin = std::nullopt;
    state = State::LineStrip;
    endLineStrip();
}

void Renderer2DScene::addLinePoint(vml::float3 p, vml::float4 color,
                                   float thickness) {
    BL_ASSERT(state == State::LineStrip || state == State::LineLoop,
              "Invalid state");
    Point current = { p, color, thickness };
    if (state == State::LineLoop && !loopBegin) {
        loopBegin = current;
    }
    auto begin = std::exchange(lastPoint, current);
    if (!begin) {
        return;
    }
    _vertices.push_back({ begin->position, current.position, 1,
                          begin->thickness, begin->color });
    _vertices.push_back({ begin->position, current.position, -1,
                          begin->thickness, begin->color });
    _vertices.push_back({ current.position, begin->position, -1,
                          current.thickness, current.color });
    _vertices.push_back({ current.position, begin->position, 1,
                          current.thickness, current.color });
    _vertices.push_back({ current.position, begin->position, -1,
                          current.thickness, current.color });
    _vertices.push_back({ begin->position, current.position, -1,
                          begin->thickness, begin->color });
}

static void fillBuffer(HardwareDevice& device, BufferHandle& buffer,
                       void const* data, size_t size) {
    if (!buffer || buffer.size() < size) {
        buffer = device.createBuffer(
            { .size = size, .storageMode = StorageMode::Managed });
    }
    device.fillManagedBuffer(buffer, data, size);
}

static RenderPipelineHandle createPipeline(
    HardwareDevice& device, RenderPassDescription const& passDesc) {
    return device.createRenderPipeline({
        .colorAttachments = passDesc.colorAttachments | transform([](auto& ca) {
        return ColorAttachmentDescription{
            .pixelFormat = ca.texture.description().pixelFormat
        };
    }) | ToSmallVector<>,
        .depthAttachmentPixelFormat =
            passDesc.depthAttachment.texture.description().pixelFormat,
        .vertexFunction = device.createFunction("Renderer2DWorldVS"),
        .fragmentFunction = device.createFunction("Renderer2DFS"),
    });
}

static DepthStencilHandle createDepthStencil(HardwareDevice& device) {
    return device.createDepthStencil({
        .depthCompareFunction = CompareFunction::Less,
        .depthWrite = true,
    });
}

void Renderer2D::drawInWorldCoords(
    Renderer2DScene const& scene, HardwareDevice& device,
    CommandBuffer& commandBuffer, BufferHandle cameraBuffer,
    RenderPassDescription const& renderPassDesc) {
    auto vertices = scene.vertices();
    fillBuffer(device, vertexBuffer, vertices.data(),
               vertices.size() * sizeof vertices[0]);
    if (!pipeline) {
        pipeline = createPipeline(device, renderPassDesc);
    }
    if (!depthStencil) {
        depthStencil = createDepthStencil(device);
    }
    auto encoder = commandBuffer.makeRenderEncoder(renderPassDesc);
    //    encoder->setTriangleFillMode(TriangleFillMode::Lines);
    encoder->setPipeline(pipeline);
    encoder->setDepthStencil(depthStencil);
    encoder->setVertexBuffer(cameraBuffer, 0);
    encoder->setVertexBuffer(vertexBuffer, 1);
    encoder->setTriangleCullMode(TriangleCullMode::None);
    encoder->draw(PrimitiveType::Triangle, 0, vertices.size());
    encoder->end();
}
