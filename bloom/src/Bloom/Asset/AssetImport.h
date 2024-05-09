#ifndef BLOOM_ASSET_MESHIMPORT_H
#define BLOOM_ASSET_MESHIMPORT_H

#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Core.h"

#include <filesystem>

namespace bloom {

struct StaticMeshData;

///
StaticMeshData importStaticMesh(std::filesystem::path const& path);

///
void importTexture(Texture2D& texture, std::filesystem::path const& path);

} // namespace bloom

#endif // BLOOM_ASSET_MESHIMPORT_H
