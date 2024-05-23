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
    _vertices.push_back({ begin, end, 1, thickness, color });
    _vertices.push_back({ begin, end, -1, thickness, color });
    _vertices.push_back({ end, begin, -1, thickness, color });
    _vertices.push_back({ begin, end, -1, thickness, color });
    _vertices.push_back({ end, begin, 1, thickness, color });
    _vertices.push_back({ end, begin, -1, thickness, color });
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
    encoder->setPipeline(pipeline);
    encoder->setDepthStencil(depthStencil);
    encoder->setVertexBuffer(cameraBuffer, 0);
    encoder->setVertexBuffer(vertexBuffer, 1);
    encoder->setTriangleCullMode(TriangleCullMode::None);
    encoder->draw(PrimitiveType::Triangle, 0, vertices.size());
    encoder->end();
}
