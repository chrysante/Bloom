#ifndef BLOOM_SCENE_COMPONENTS_TRANSFORM_H
#define BLOOM_SCENE_COMPONENTS_TRANSFORM_H

#include <vml/vml.hpp>

#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct BLOOM_API Transform {
    BLOOM_REGISTER_COMPONENT("Transform")

    vml::float3 position = 0;
    vml::quaternion_float orientation = 1;
    vml::float3 scale = 1;

    static Transform fromMatrix(vml::float4x4 const& m) {
        auto const [t, r, s] = vml::decompose_transform(m);
        return { t, r, s };
    }

    [[nodiscard]] vml::float4x4 calculate() const {
        return vml::make_transform(position, orientation, scale);
    }
};

struct BLOOM_API TransformMatrixComponent {
    BLOOM_REGISTER_COMPONENT("Transform Matrix")
    vml::float4x4 matrix;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::Transform, position, orientation, scale);

#endif

#endif // BLOOM_SCENE_COMPONENTS_TRANSFORM_H
