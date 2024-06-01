#ifndef BLOOM_SCENE_COMPONENTS_LIGHTS_H
#define BLOOM_SCENE_COMPONENTS_LIGHTS_H

#include "Bloom/Core/Base.h"
#include "Bloom/Graphics/Lights.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct BLOOM_API PointLightComponent: PointLight {
    using Light = PointLight;
};

struct BLOOM_API SpotLightComponent: SpotLight {
    using Light = SpotLight;
};

struct BLOOM_API DirectionalLightComponent: DirectionalLight {
    using Light = DirectionalLight;
};

struct BLOOM_API SkyLightComponent: SkyLight {
    using Light = SkyLight;
};

/// Type list of all light components
using LightComponentList =
    TypeList<PointLightComponent, SpotLightComponent, DirectionalLightComponent,
             SkyLightComponent>;

} // namespace bloom

#include "Bloom/Core/Yaml.h"

template <>
struct YAML::convert<bloom::PointLightComponent> {
    static Node encode(bloom::PointLightComponent const& l) {
        Node node;
        node = (bloom::PointLight const&)l;
        return node;
    }

    static bool decode(Node const& node, bloom::PointLightComponent& m) {
        (bloom::PointLight&)m = node.as<bloom::PointLight>();
        return true;
    }
};

template <>
struct YAML::convert<bloom::SpotLightComponent> {
    static Node encode(bloom::SpotLightComponent const& l) {
        Node node;
        node = (bloom::SpotLight const&)l;
        return node;
    }

    static bool decode(Node const& node, bloom::SpotLightComponent& m) {
        (bloom::SpotLight&)m = node.as<bloom::SpotLight>();
        return true;
    }
};

template <>
struct YAML::convert<bloom::DirectionalLightComponent> {
    static Node encode(bloom::DirectionalLightComponent const& l) {
        Node node;
        node = (bloom::DirectionalLight const&)l;
        return node;
    }

    static bool decode(Node const& node, bloom::DirectionalLightComponent& m) {
        (bloom::DirectionalLight&)m = node.as<bloom::DirectionalLight>();
        return true;
    }
};

template <>
struct YAML::convert<bloom::SkyLightComponent> {
    static Node encode(bloom::SkyLightComponent const& l) {
        Node node;
        node = (bloom::SkyLight const&)l;
        return node;
    }

    static bool decode(Node const& node, bloom::SkyLightComponent& m) {
        (bloom::SkyLight&)m = node.as<bloom::SkyLight>();
        return true;
    }
};

#endif // BLOOM_SCENE_COMPONENTS_LIGHTS_H
