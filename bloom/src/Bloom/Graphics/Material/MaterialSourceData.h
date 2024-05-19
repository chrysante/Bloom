#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALSOURCEDATA_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALSOURCEDATA_H

#include <string>
#include <vector>

#include <utl/hashtable.hpp>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Graphics/Material/MaterialParameter.h"

namespace bloom {

struct MaterialSourceData {
    std::string source;
    utl::hashmap<int, AssetHandle> constantFragmentTextures;
    std::vector<MaterialPrimitiveParameter> primitiveParameters;
    std::vector<MaterialTextureParameter> textureParameters;
    size_t parameterBufferSize;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALSOURCEDATA_H
