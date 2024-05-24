#include "Bloom/Graphics/Lights.h"

using namespace bloom;

PointLight PointLight::Default() {
    return { .common = { .color = { 1, 1, 1 }, .intensity = 1'000 },
             .position = { 0, 0, 0 },
             .radius = 10,
             .falloff = 1 };
}

SpotLight SpotLight::Default() {
    return { .common = { .color = { 1, 1, 1 }, .intensity = 1'000 },
             .position = { 0, 0, 0 },
             .direction = { 1, 0, 0 },
             .innerCutoff = 0,
             .outerCutoff = 0,
             .radius = 10 };
}

DirectionalLight DirectionalLight::Default() {
    return { .common = { .color = { 1, 1, 1 }, .intensity = 1 },
             .direction = { 0, 0, 1 },
             .castsShadows = true,
             .numCascades = 5,
             .shadowDistance = 500,
             .shadowDistanceZ = 10'000,
             .cascadeDistributionExponent = 2,
             .cascadeTransitionFraction = 0.05,
             .shadowDistanceFadeoutFraction = 0.05 };
}

SkyLight SkyLight::Default() {
    return {
        .common = { .color = { 1, 1, 1 }, .intensity = 0.4 },
    };
}
