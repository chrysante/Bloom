#ifndef BLOOM_GRAPHICS_LIGHTS_H
#define BLOOM_GRAPHICS_LIGHTS_H

#include "Bloom/Asset/Serialization.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Shader/Lights.h"

namespace bloom {

struct PointLight: PointLightRenderData {
    static PointLight Default();
};

struct SpotLight: SpotLightRenderData {
    static SpotLight Default();
};

struct DirectionalLight: DirectionalLightRenderData {
    static DirectionalLight Default();
};

struct SkyLight: SkyLightRenderData {
    Reference<Texture2D> cubemap;

    static SkyLight Default();
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::PointLight, color, intensity, position,
                           radius, falloff);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SpotLight, color, intensity, position,
                           direction, innerCutoff, outerCutoff, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::DirectionalLight, color, intensity, direction,
                           castsShadows, numCascades, shadowDistance,
                           shadowDistanceZ, cascadeDistributionExponent,
                           cascadeTransitionFraction,
                           shadowDistanceFadeoutFraction);

template <>
struct YAML::convert<bloom::SkyLight> {
    static Node encode(bloom::SkyLight const& light);
    static bool decode(Node const& node, bloom::SkyLight& light);
};

#endif // BLOOM_GRAPHICS_LIGHTS_H
