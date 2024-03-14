#include "Bloom/Asset/MeshImport.hpp"

#include <fstream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <utl/strcat.hpp>

#include "Bloom/Graphics/StaticMesh.hpp"

using namespace bloom;

/// Maybe move this somewhere else
static float BaseWorldScale() { return 100; }

StaticMeshData bloom::importStaticMesh(std::filesystem::path const& path) {
    std::fstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error(utl::strcat("Failed to open file ", path));
    }

    std::stringstream sstr;
    sstr << file.rdbuf();
    std::string data = sstr.str();

    {
        Assimp::Importer importer;
        importer.ReadFileFromMemory(data.data(), data.size(),
                                    aiProcess_Triangulate |
                                        aiProcess_CalcTangentSpace |
                                        aiProcess_JoinIdenticalVertices |
                                        aiProcess_FlipWindingOrder);
        auto* const scene = importer.GetScene();
        Logger::Trace("File ", path, " contains ", scene->mNumMeshes,
                      " mesh(es)");
        assert(scene->mNumMeshes);

        auto* const mesh = scene->mMeshes[0];
        bloom::StaticMeshData result;

        auto* const position = mesh->mVertices;
        assert(position);
        auto* const normal = mesh->mNormals;
        auto* const tangent = mesh->mTangents;
        auto* const color = mesh->mColors[0];
        std::array const uv = { mesh->mTextureCoords[0],
                                mesh->mTextureCoords[1],
                                mesh->mTextureCoords[2],
                                mesh->mTextureCoords[3] };
        {
            std::size_t const vertexCount = mesh->mNumVertices;
            result.vertices.resize(vertexCount);

            for (std::size_t i = 0; i < vertexCount; ++i) {
                auto& v = result.vertices[i];
                if (position)
                    v.position = BaseWorldScale() *
                                 mtl::float3{ position[i].x, position[i].y,
                                              position[i].z };
                if (normal)
                    v.normal = { normal[i].x, normal[i].y, normal[i].z };
                if (tangent)
                    v.tangent = { tangent[i].x, tangent[i].y, tangent[i].z };
                if (color)
                    v.color = { color[i].r, color[i].g, color[i].b,
                                color[i].a };
                for (int j = 0; j < 4; ++j) {
                    if (uv[j])
                        v.textureCoordinates[j] = { uv[j][i].x, uv[j][i].y };
                }
            }
        }
        {
            std::size_t const faceCount = mesh->mNumFaces;
            std::size_t const indexCount = faceCount * 3;
            result.indices.resize(indexCount);
            auto* const face = mesh->mFaces;
            for (std::size_t i = 0, k = 0; i < faceCount; ++i) {
                assert(face[i].mNumIndices == 3);
                for (std::size_t j = 0; j < 3; ++j, ++k) {
                    result.indices[k] = face[i].mIndices[j];
                }
            }
        }
        return result;
    }
}