#ifndef BLOOM_GRAPHICS_LIGHTS_H
#define BLOOM_GRAPHICS_LIGHTS_H

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Core/Yaml.h"
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
    AssetHandle cubemap;

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

BLOOM_MAKE_TEXT_SERIALIZER(bloom::SkyLight, color, intensity);

#endif // BLOOM_GRAPHICS_LIGHTS_H
