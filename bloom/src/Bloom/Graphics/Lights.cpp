#include "Bloom/Graphics/Lights.h"

using namespace bloom;

PointLight PointLight::Default() {
    return { { .color = { 1, 1, 1 },
               .intensity = 1'000,
               .position = { 0, 0, 0 },
               .radius = 10,
               .falloff = 1 } };
}

SpotLight SpotLight::Default() {
    return { { .color = { 1, 1, 1 },
               .intensity = 1'000,
               .position = { 0, 0, 0 },
               .direction = { 1, 0, 0 },
               .innerCutoff = 0,
               .outerCutoff = 0,
               .radius = 10 } };
}

DirectionalLight DirectionalLight::Default() {
    return { { .color = { 1, 1, 1 },
               .intensity = 1,
               .direction = { 0, 0, 1 },
               .castsShadows = true,
               .numCascades = 5,
               .shadowDistance = 500,
               .shadowDistanceZ = 10'000,
               .cascadeDistributionExponent = 2,
               .cascadeTransitionFraction = 0.05,
               .shadowDistanceFadeoutFraction = 0.05 } };
}

SkyLight SkyLight::Default() {
    return { { .color = { 1, 1, 1 }, .intensity = 0.4 }, .cubemap = nullptr };
}

YAML::Node YAML::convert<bloom::SkyLight>::encode(SkyLight const& light) {
    Node node;
    node["color"] = light.color;
    node["intensity"] = light.intensity;
    node["cubemap"] = light.cubemap ? light.cubemap->handle() : AssetHandle{};
    return node;
}

bool YAML::convert<bloom::SkyLight>::decode(Node const& node, SkyLight& light) {
    light.color = node["color"].as<vml::float3>(vml::float3{});
    light.intensity = node["intensity"].as<float>(0);
    using enum AssetRepresentation;
    light.cubemap =
        getAsset<bloom::Texture2D>(node["cubemap"].as<bloom::AssetHandle>(
                                       AssetHandle{}),
                                   GPU | CPU);
    return true;
}
