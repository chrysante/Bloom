#include "Bloom/Asset/AssetImport.h"

#include <fstream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/STBImage.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Graphics/Texture.h"

using namespace bloom;
using namespace vml::short_types;

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
        if (normal) v.normal = { normal[i].x, normal[i].y, normal[i].z };
        if (tangent) v.tangent = { tangent[i].x, tangent[i].y, tangent[i].z };
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

void bloom::importTexture(Texture2D& texture,
                          std::filesystem::path const& path) {
    int2 size{};
    int channelsInFile = 0;
    auto* buffer =
        stbi_load(path.c_str(), &size.x, &size.y, &channelsInFile, 4);
    if (!buffer) {
        throw std::runtime_error("Failed to import texture");
    }
    size_t bufferSize = size.x * size.y * 4;
    texture.size = size;
    texture.data.assign(buffer, buffer + bufferSize);
    texture.pixelFormat = TexturePixelFormat::RGBA8;
    stbi_image_free(buffer);
}
