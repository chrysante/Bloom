#include "Bloom/Asset/AssetImport.h"

#include <fstream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <range/v3/view.hpp>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/CubemapRenderer.h"
#include "Bloom/Asset/STBImage.h"
#include "Bloom/Asset/TinyEXR.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Graphics/Texture.h"

using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

/// Maybe move this somewhere else
static float BaseWorldScale() { return 100; }

static std::vector<uint8_t> readFileToMemory(
    std::filesystem::path const& path) {
    std::fstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(utl::strcat("Failed to open file: ", path));
    }
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!file.read((char*)buffer.data(), size)) {
        throw std::runtime_error(
            utl::strcat("Failed read file contents: ", path));
    }
    return buffer;
}

StaticMeshData bloom::importStaticMesh(std::filesystem::path const& path) {
    auto buffer = readFileToMemory(path);
    Assimp::Importer importer;
    importer.ReadFileFromMemory(buffer.data(), buffer.size(),
                                aiProcess_Triangulate |
                                    aiProcess_CalcTangentSpace |
                                    aiProcess_JoinIdenticalVertices |
                                    aiProcess_FlipWindingOrder);
    auto* scene = importer.GetScene();
    if (!scene) {
        throw std::runtime_error("Failed to import, scene is empty");
    }
    Logger::Trace("File ", path, " contains ", scene->mNumMeshes, " mesh(es)");
    assert(scene->mNumMeshes);
    auto* mesh = scene->mMeshes[0];
    bloom::StaticMeshData result;
    auto* position = mesh->mVertices;
    BL_ASSERT(position);
    auto* normal = mesh->mNormals;
    auto* tangent = mesh->mTangents;
    auto* binormal = mesh->mBitangents;
    auto* color = mesh->mColors[0];
    std::array uv = { mesh->mTextureCoords[0], mesh->mTextureCoords[1],
                      mesh->mTextureCoords[2], mesh->mTextureCoords[3] };
    size_t vertexCount = mesh->mNumVertices;
    result.vertices.resize(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        auto& v = result.vertices[i];
        if (position)
            v.position =
                BaseWorldScale() *
                vml::float3{ position[i].x, position[i].y, position[i].z };
        if (normal) {
            v.normal = { normal[i].x, normal[i].y, normal[i].z };
        }
        if (tangent) {
            v.tangent = { tangent[i].x, tangent[i].y, tangent[i].z };
        }
        if (binormal) {
            v.binormal = { binormal[i].x, binormal[i].y, binormal[i].z };
        }
        if (color) v.color = { color[i].r, color[i].g, color[i].b, color[i].a };
        for (int j = 0; j < 4; ++j) {
            if (uv[j]) v.textureCoordinates[j] = { uv[j][i].x, uv[j][i].y };
        }
    }
    size_t faceCount = mesh->mNumFaces;
    size_t indexCount = faceCount * 3;
    result.indices.resize(indexCount);
    auto* const face = mesh->mFaces;
    for (size_t i = 0, k = 0; i < faceCount; ++i) {
        BL_ASSERT(face[i].mNumIndices == 3);
        for (size_t j = 0; j < 3; ++j, ++k) {
            result.indices[k] = face[i].mIndices[j];
        }
    }
    return result;
}

static void importTextureGeneric(Texture2D& texture,
                                 std::filesystem::path const& path,
                                 PixelFormat pixelFormat) {
    switch (pixelFormat) {
    case PixelFormat::RGBA8Unorm: {
        int2 size{};
        int channelsInFile = 0;
        uint8_t* buffer =
            stbi_load(path.c_str(), &size.x, &size.y, &channelsInFile, 4);
        if (!buffer) {
            Logger::Error("Failed to import texture: ", stbi_failure_reason());
            return;
        }
        size_t bufferSize = size.x * size.y * 4;
        texture.type = TextureType::Texture2D;
        texture.pixelFormat = PixelFormat::RGBA8Unorm;
        texture.size = size;
        texture.pixelData = std::make_unique<uint8_t[]>(bufferSize);
        std::memcpy(texture.pixelData.get(), buffer, bufferSize);
        stbi_image_free(buffer);
        return;
    }
    case PixelFormat::RGBA32Float: {
        if (!stbi_is_hdr(path.c_str())) {
            Logger::Error("File is not HDR format");
            return;
        }
        int2 size{};
        int channelsInFile = 0;
        float* buffer =
            stbi_loadf(path.c_str(), &size.x, &size.y, &channelsInFile, 4);
        if (!buffer) {
            Logger::Error("Failed to import texture: ", stbi_failure_reason());
            return;
        }
        texture.type = TextureType::Texture2D;
        texture.pixelFormat = PixelFormat::RGBA32Float;
        size_t numPixels = size.x * size.y;
        texture.size = size;
        texture.pixelData = std::make_unique<uint8_t[]>(numPixels * 16);
        std::memcpy(texture.pixelData.get(), buffer, numPixels * 16);
        stbi_image_free(buffer);
        return;
    }
    default:
        BL_UNIMPLEMENTED();
    }
}

[[maybe_unused]] static void importTextureEXR(Texture2D&,
                                              std::filesystem::path const&) {
    Logger::Error("EXR import unimplemented");
}

static void postprocessHDR(Texture2D& texture, HardwareDevice& device,
                           CommandQueue& commandQueue) {
    usize2 sourceSize = texture.size;
    usize2 destSize = usize2(512, 512); // Hard coded for now
    auto cubemap =
        device.createTexture({ .type = TextureType::CubeTexture,
                               .pixelFormat = PixelFormat::RGBA32Float,
                               .size = { destSize, 1 },
                               .usage = TextureUsage::RenderTarget,
                               .storageMode = StorageMode::Managed });
    auto equirectangular =
        device.createTexture({ .type = TextureType::Texture2D,
                               .pixelFormat = PixelFormat::RGBA32Float,
                               .size = { sourceSize, 1 },
                               .storageMode = StorageMode::GPUOnly });
    size_t numPixelSourceBytes = texture.getPixelDataByteSize();
    auto textureDataBuffer =
        device.createBuffer({ .data = texture.pixelData.get(),
                              .size = numPixelSourceBytes,
                              .storageMode = StorageMode::Managed });
    auto commandBuffer = commandQueue.createCommandBuffer();
    auto toTexEncoder = commandBuffer->makeBlitEncoder();
    toTexEncoder->copyBufferToTexture(textureDataBuffer, 0, sourceSize.x * 16,
                                      numPixelSourceBytes, { sourceSize, 1 },
                                      equirectangular, 0, 0, { 0, 0, 0 });
    toTexEncoder->end();
    renderCubemap(device, *commandBuffer, cubemap, equirectangular);
    auto syncEncoder = commandBuffer->makeBlitEncoder();
    syncEncoder->synchronizeTexture(cubemap, 0, 0);
    syncEncoder->end();
    commandBuffer->commit();
    commandBuffer->waitForCompletion();
    size_t bytesPerImage = destSize.x * destSize.y * 16;
    texture.pixelData = std::make_unique<uint8_t[]>(bytesPerImage * 6);
    uint8_t* destBuffer = texture.pixelData.get();
    for (size_t i = 0; i < 6; ++i) {
        copyTextureToMemory(RenderAPI::Metal, cubemap,
                            destBuffer + i * bytesPerImage,
                            /* bytesPerRow: */ destSize.x * 16, bytesPerImage,
                            { usize3{}, usize3{ destSize, 1 } },
                            /* mipLevel: */ 0, /* slice: */ i);
    }
    texture.pixelFormat = PixelFormat::RGBA32Float;
    texture.size = destSize;
    texture.type = TextureType::CubeTexture;
}

void bloom::importTexture(Texture2D& texture, std::filesystem::path const& path,
                          HardwareDevice& device, CommandQueue& commandQueue) {
    bool isHDR = path.extension() == ".hdr";
    importTextureGeneric(texture, path,
                         isHDR ? PixelFormat::RGBA32Float :
                                 PixelFormat::RGBA8Unorm);
    if (isHDR) {
        postprocessHDR(texture, device, commandQueue);
    }
}
