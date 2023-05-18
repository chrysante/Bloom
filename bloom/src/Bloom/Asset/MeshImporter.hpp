#pragma once

#include "Bloom/Core/Core.hpp"

#include <filesystem>

namespace bloom {

class BLOOM_API StaticMeshData;

class MeshImporter {
public:
    StaticMeshData import(std::filesystem::path);

private:
};

} // namespace bloom
