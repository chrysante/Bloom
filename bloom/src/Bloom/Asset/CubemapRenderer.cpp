#include "Bloom/Asset/CubemapRenderer.h"

#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"

using namespace bloom;
using namespace vml::short_types;

void bloom::renderCubemap(HardwareDevice& device, CommandBuffer& commandBuffer,
                          TextureHandle cubemap,
                          TextureHandle equirectangular) {
    BL_ASSERT(cubemap.description().type == TextureType::CubeTexture);
    auto pipeline = device.createRenderPipeline({
        .colorAttachments = { {
            .pixelFormat = cubemap.description().pixelFormat,
        } },
        .vertexFunction = device.createFunction("cubemapRendererVS"),
        .fragmentFunction = device.createFunction("cubemapRendererFS"),
    });
    float4x4 projection = vml::perspective(vml::constants<>::pi / 2,
                                           /* aspectRatio: */ 1.0f,
                                           /* near: */ 0.1f,
                                           /* far: */ 10.0f);
    std::array<float4x4, 6> viewMatrices = {
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 0.0f, 0.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(-1.0f, 0.0f, 0.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f),
                     float3(0.0f, 0.0f, 1.0f)),
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f),
                     float3(0.0f, 0.0f, -1.0f)),
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, -1.0f),
                     float3(0.0f, -1.0f, 0.0f))
    };
    for (auto& m: viewMatrices) {
        m = vml::transpose(projection * m);
    }
    auto cameraBuffer =
        device.createBuffer({ .data = viewMatrices.data(),
                              .size = sizeof viewMatrices,
                              .storageMode = StorageMode::Managed });
    for (size_t slice = 0; slice < 6; ++slice) {
        TextureHandle side =
            createSharedTextureView(RenderAPI::Metal, cubemap,
                                    TextureType::Texture2D,
                                    cubemap.description().pixelFormat,
                                    /* firstMipLevel: */ 0,
                                    /* numMipLevels: */ 1,
                                    /* firstSlice: */ slice,
                                    /* numSlices: */ 1);
        auto encoder = commandBuffer.makeRenderEncoder({
            .colorAttachments = { {
                .texture = side,
            } },
            .renderTargetSize = cubemap.size().xy,
        });
        encoder->setPipeline(pipeline);
        encoder->setVertexBuffer(cameraBuffer, 0, sizeof(float4x4) * slice);
        encoder->setFragmentTexture(equirectangular, 0);
        encoder->setTriangleCullMode(TriangleCullMode::None);
        encoder->draw(PrimitiveType::TriangleStrip, 4 * slice, 4);
        encoder->end();
    }
}
