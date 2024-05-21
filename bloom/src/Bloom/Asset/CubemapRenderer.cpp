#include "Bloom/Asset/CubemapRenderer.h"

#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/ShaderCompiler.h"

using namespace bloom;
using namespace vml::short_types;

static void renderCube(CommandBuffer& commandBuffer, HardwareDevice& device,
                       RenderPipelineHandle& pipeline, TextureView renderTarget,
                       TextureView source) {
    float4x4 projection = vml::perspective(vml::constants<>::pi / 2,
                                           /* aspectRatio: */ 1.0f,
                                           /* near: */ 0.1f,
                                           /* far: */ 10.0f);
    std::array<float4x4, 6> viewMatrices = {
        // +X
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 0.0f, 0.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        // -X
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(-1.0f, 0.0f, 0.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        // -Y
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f),
                     float3(0.0f, 0.0f, -1.0f)),
        // +Y
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f),
                     float3(0.0f, 0.0f, 1.0f)),
        // +Z
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        // -Z
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, -1.0f),
                     float3(0.0f, -1.0f, 0.0f)),
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
            createSharedTextureView(RenderAPI::Metal, renderTarget,
                                    TextureType::Texture2D,
                                    renderTarget.description().pixelFormat,
                                    /* firstMipLevel: */ 0,
                                    /* numMipLevels: */ 1,
                                    /* firstSlice: */ slice,
                                    /* numSlices: */ 1);
        auto encoder = commandBuffer.makeRenderEncoder({
            .colorAttachments = { {
                .texture = side,
            } },
            .renderTargetSize = renderTarget.size().xy,
        });
        encoder->setPipeline(pipeline);
        encoder->setVertexBuffer(cameraBuffer, 0, sizeof(float4x4) * slice);
        encoder->setFragmentTexture(source, 0);
        encoder->setTriangleCullMode(TriangleCullMode::None);
        encoder->draw(PrimitiveType::TriangleStrip, 4 * slice, 4);
        encoder->end();
    }
}

void bloom::renderCubemap(HardwareDevice& device, CommandBuffer& commandBuffer,
                          TextureHandle cubemap,
                          TextureHandle equirectangular) {
    BL_ASSERT(cubemap.description().type == TextureType::CubeTexture);
    auto shaderCompiler = device.createShaderCompiler();
    auto functions = shaderCompiler->compile(
        std::filesystem::path(
            "/Users/indigo/dev/bloom/poppy/resource/Shader/CubemapRenderer.metal"),
        std::vector<std::string>{ "cubeRendererVS", "cubemapRendererFS" });
    if (functions.empty()) {
        return;
    }
    auto pipeline = device.createRenderPipeline({
        .colorAttachments = { {
            .pixelFormat = cubemap.description().pixelFormat,
        } },
        .vertexFunction = functions[0],
        .fragmentFunction = functions[1],
    });
    renderCube(commandBuffer, device, pipeline, cubemap, equirectangular);
}

void bloom::renderIrradianceMap(HardwareDevice& device,
                                CommandBuffer& commandBuffer,
                                TextureHandle dest, TextureHandle source) {
    BL_ASSERT(source.description().type == TextureType::CubeTexture);
    BL_ASSERT(dest.description().type == TextureType::CubeTexture);
    auto shaderCompiler = device.createShaderCompiler();
    auto functions = shaderCompiler->compile(
        std::filesystem::path(
            "/Users/indigo/dev/bloom/poppy/resource/Shader/CubemapRenderer.metal"),
        std::vector<std::string>{ "cubeRendererVS", "irradianceRendererFS" });
    if (functions.empty()) {
        return;
    }
    auto pipeline = device.createRenderPipeline({
        .colorAttachments = { {
            .pixelFormat = dest.description().pixelFormat,
        } },
        .vertexFunction = functions[0],
        .fragmentFunction = functions[1],
    });
    renderCube(commandBuffer, device, pipeline, dest, source);
}

static float3 mapUVSToN(float u, float v, size_t face) {
    switch (face) {
    case 0: // +X
        return vml::normalize(float3(1, -v, -u));
    case 1: // -X
        return vml::normalize(float3(-1, -v, u));
    case 2: // +Y
        return vml::normalize(float3(u, 1, v));
    case 3: // -Y
        return vml::normalize(float3(u, -1, -v));
    case 4: // +Z
        return vml::normalize(float3(u, -v, 1));
    case 5: // -Z
        return vml::normalize(float3(-u, -v, -1));
    default:
        BL_UNREACHABLE();
    }
}

static std::array<float, 9> genSHCoefficients(float3 N) {
    return { // Band 0
             0.282095f,
             // Band 1
             0.488603f * N.y, 0.488603f * N.z, 0.488603f * N.x,
             // Band 2
             1.092548f * N.x * N.y, 1.092548f * N.y * N.z,
             0.315392f * (3.0f * N.z * N.z - 1.0f), 1.092548f * N.x * N.z,
             0.546274f * (N.x * N.x - N.y * N.y)
    };
}

static std::array<float3, 9> genLightingCoefficientsForNormal(float3 N,
                                                              float3 L) {
    auto coeff = genSHCoefficients(N);
    vml::vector<float3, 9> result;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = L * coeff[i];
    }
    return result;
}

std::array<float3, 9> bloom::genLightingCoefficients(float4 const* imageData,
                                                     usize2 resolution,
                                                     float clamp) {
    vml::vector<float3, 9> result{};
    float weightSum = 0;
    for (size_t s = 0; s < 6; ++s) {
        float4 const* sideData = imageData + s * resolution.x * resolution.y;
        for (size_t y = 0; y < resolution.y; ++y) {
            for (size_t x = 0; x < resolution.x; ++x) {
                float3 L =
                    vml::min(sideData[y * resolution.x + x].xyz, float3(clamp));
                float2 coords = 2 * (float2(x, y) + 0.5f) / resolution - 1;
                float temp = 1.0f + vml::length_squared(coords);
                float weight = 4.0f / (std::sqrt(temp) * temp);
                float3 N = mapUVSToN(coords.x, coords.y, s);
                auto coeff = genLightingCoefficientsForNormal(N, L);
                for (size_t i = 0; i < result.size(); ++i) {
                    result[i] += coeff[i] * weight;
                }
                weightSum += weight;
            }
        }
    }
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] *= 4.0f * vml::constants<float>::pi / weightSum;
    }
    return result;
}
