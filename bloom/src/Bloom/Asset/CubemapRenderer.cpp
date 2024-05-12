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
    size_t slice = 0;
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
    float4x4 projection = vml::perspective(vml::constants<>::pi / 2,
                                           /* aspectRatio: */ 1.0f,
                                           /* near: */ 0.1f,
                                           /* far: */ 10.0f);
    float4x4 view = vml::look_at(/* eye: */ float3(0.0f, 0.0f, 0.0f),
                                 /* center: */ float3(1.0f, 0.0f, 0.0f),
                                 /* up: */ float3(0.0f, -1.0f, 0.0f));
    float4x4 camera = vml::transpose(projection * view);
    auto cameraBuffer =
        device.createBuffer({ .data = &camera, .size = sizeof camera });
    encoder->setVertexBuffer(cameraBuffer, 0);
    encoder->setFragmentTexture(equirectangular, 0);
    encoder->draw(PrimitiveType::TriangleStrip, 0, 4);
}
