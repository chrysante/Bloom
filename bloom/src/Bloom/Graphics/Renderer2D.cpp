#include "Bloom/Graphics/Renderer2D.h"

#include <range/v3/view.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

void Renderer2D::begin() {
    scenes.clear();
    vertices.clear();
}

void Renderer2D::beginScene() { scenes.push_back({ vertices.size(), 0 }); }

void Renderer2D::endScene() {}

void Renderer2D::line(float3 begin, float3 end, float4 color, float thickness) {
    vertices.push_back({ begin, end, 1, thickness, color });
    vertices.push_back({ begin, end, -1, thickness, color });
    vertices.push_back({ end, begin, -1, thickness, color });
    vertices.push_back({ begin, end, -1, thickness, color });
    vertices.push_back({ end, begin, 1, thickness, color });
    vertices.push_back({ end, begin, -1, thickness, color });
    current().vertexCount += 6;
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

void Renderer2D::draw(BufferHandle cameraBuffer, HardwareDevice& device,
                      CommandBuffer& commandBuffer,
                      RenderPassDescription const& renderPassDesc) {
    fillBuffer(device, vertexBuffer, vertices.data(),
               vertices.size() * sizeof(LineVertex2D));
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
    for (auto [vertexBegin, vertexCount]: scenes) {
        encoder->draw(PrimitiveType::Triangle, vertexBegin, vertexCount);
    }
    encoder->end();
}

Renderer2DScene::reference Renderer2D::current() {
    BL_ASSERT(!scenes.empty(), "No current scene");
    return scenes.back();
}
