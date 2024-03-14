#pragma once

#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Scene/Components/ComponentBase.hpp"

namespace bloom {

struct BLOOM_API PointLightComponent {
    BLOOM_REGISTER_COMPONENT("Point Light")

    PointLight light;
};

struct BLOOM_API SpotLightComponent {
    BLOOM_REGISTER_COMPONENT("Spot Light")

    SpotLight light;
};

struct BLOOM_API DirectionalLightComponent {
    BLOOM_REGISTER_COMPONENT("Directional Light")

    DirectionalLight light;
};

struct BLOOM_API SkyLightComponent {
    BLOOM_REGISTER_COMPONENT("Sky Light")

    SkyLight light;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

template <>
struct YAML::convert<bloom::PointLightComponent> {
    static Node encode(bloom::PointLightComponent const& l) {
        Node node;
        node = l.light;
        return node;
    }

    static bool decode(Node const& node, bloom::PointLightComponent& m) {
        m.light = node.as<bloom::PointLight>();
        return true;
    }
};

template <>
struct YAML::convert<bloom::SpotLightComponent> {
    static Node encode(bloom::SpotLightComponent const& l) {
        Node node;
        node = l.light;
        return node;
    }

    static bool decode(Node const& node, bloom::SpotLightComponent& m) {
        m.light = node.as<bloom::SpotLight>();
        return true;
    }
};

template <>
struct YAML::convert<bloom::DirectionalLightComponent> {
    static Node encode(bloom::DirectionalLightComponent const& l) {
        Node node;
        node = l.light;
        return node;
    }

    static bool decode(Node const& node, bloom::DirectionalLightComponent& m) {
        m.light = node.as<bloom::DirectionalLight>();
        return true;
    }
};

template <>
struct YAML::convert<bloom::SkyLightComponent> {
    static Node encode(bloom::SkyLightComponent const& l) {
        Node node;
        node = l.light;
        return node;
    }

    static bool decode(Node const& node, bloom::SkyLightComponent& m) {
        m.light = node.as<bloom::SkyLight>();
        return true;
    }
};

#endif
