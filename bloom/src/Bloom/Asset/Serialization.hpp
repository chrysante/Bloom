#ifndef BLOOM_ASSET_SERIALIZATION_H
#define BLOOM_ASSET_SERIALIZATION_H

#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/AssetHandle.hpp"
#include "Bloom/Asset/Fwd.hpp"
#include "Bloom/Core/Yaml.hpp"

template <>
struct YAML::convert<bloom::AssetType> {
    static Node encode(bloom::AssetType type) { return Node(toString(type)); }

    static bool decode(Node const& node, bloom::AssetType& type) {
        auto str = node.as<std::string>();
#define BLOOM_ASSET_TYPE_DEF(Name, ...)                                        \
    if (str == #Name) {                                                        \
        type = bloom::AssetType::Name;                                         \
        return true;                                                           \
    }
#include "Bloom/Asset/Assets.def"
        return false;
    }
};

template <>
struct YAML::convert<bloom::AssetHandle> {
    static Node encode(bloom::AssetHandle const& handle) {
        Node node;
        node["Type"] = handle.type();
        node["ID"] = handle.ID();
        return node;
    }

    static bool decode(Node const& node, bloom::AssetHandle& handle) {
        handle = bloom::AssetHandle(node["Type"].as<bloom::AssetType>(),
                                    node["ID"].as<utl::uuid>());
        return true;
    }
};

#endif // BLOOM_ASSET_SERIALIZATION_H
