#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETER_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETER_H

#include <string>

#include <vml/vml.hpp>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Graphics/Material/Fwd.h"

namespace bloom {

struct MaterialPrimitiveParameter {
    MaterialDataType dataType;
    uint32_t bufferOffset;
    std::string name;
    vml::float4 defaultValue;
};

struct MaterialTextureParameter {
    static constexpr int UnusedBindingPoint = -1;

    MaterialDataType type;
    int fragmentBindingPoint = UnusedBindingPoint;
    int vertexBindingPoint = UnusedBindingPoint;
    std::string name;
    AssetHandle defaultValue;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETER_H
