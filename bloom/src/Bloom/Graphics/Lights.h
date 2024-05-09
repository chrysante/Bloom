#ifndef BLOOM_GRAPHICS_LIGHTS_H
#define BLOOM_GRAPHICS_LIGHTS_H

#include "Bloom/Core/Serialize.h"
#include "Bloom/Shader/Lights.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::LightCommon, color, intensity);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PointLight, common, position, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SpotLight, common, position, direction,
                           innerCutoff, outerCutoff, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::DirectionalLight, common, direction,
                           castsShadows, numCascades, shadowDistance,
                           shadowDistanceZ, cascadeDistributionExponent,
                           cascadeTransitionFraction,
                           shadowDistanceFadeoutFraction);

BLOOM_MAKE_TEXT_SERIALIZER(bloom::SkyLight, common);

#endif // BLOOM_GRAPHICS_LIGHTS_H
