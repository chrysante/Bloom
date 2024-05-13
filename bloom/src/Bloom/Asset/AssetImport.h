#ifndef BLOOM_ASSET_ASSETIMPORT_H
#define BLOOM_ASSET_ASSETIMPORT_H

#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Core.h"
#include "Bloom/GPU/Fwd.h"

#include <filesystem>

namespace bloom {

struct StaticMeshData;

///
StaticMeshData importStaticMesh(std::filesystem::path const& path);

///
void importTexture(Texture2D& texture, std::filesystem::path const& path,
                   HardwareDevice& device, CommandQueue& commandQueue);

} // namespace bloom

#endif // BLOOM_ASSET_ASSETIMPORT_H
