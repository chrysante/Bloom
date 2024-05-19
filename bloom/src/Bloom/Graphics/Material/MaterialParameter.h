#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETER_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETER_H

#include <string>

#include <vml/vml.hpp>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/Serialization.h"
#include "Bloom/Core/Serialize.h"
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

    bool hasFragmentBinding() const {
        return fragmentBindingPoint != UnusedBindingPoint;
    }
    bool hasVertexBinding() const {
        return vertexBindingPoint != UnusedBindingPoint;
    }

    MaterialDataType type;
    int fragmentBindingPoint = UnusedBindingPoint;
    int vertexBindingPoint = UnusedBindingPoint;
    std::string name;
    AssetHandle defaultValue;
};

} // namespace bloom

#ifndef BLOOM_NO_SERIALIZATION

BLOOM_MAKE_TEXT_SERIALIZER(bloom::MaterialPrimitiveParameter, dataType,
                           bufferOffset, name, defaultValue);

BLOOM_MAKE_TEXT_SERIALIZER(bloom::MaterialTextureParameter, type,
                           fragmentBindingPoint, vertexBindingPoint, name,
                           defaultValue);

#endif // BLOOM_NO_SERIALIZATION

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETER_H
