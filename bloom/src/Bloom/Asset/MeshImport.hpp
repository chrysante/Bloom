#ifndef BLOOM_ASSET_MESHIMPORT_H
#define BLOOM_ASSET_MESHIMPORT_H

#include "Bloom/Core/Core.hpp"

#include <filesystem>

namespace bloom {

struct StaticMeshData;

///
StaticMeshData importStaticMesh(std::filesystem::path const& path);

} // namespace bloom

#endif // BLOOM_ASSET_MESHIMPORT_H
