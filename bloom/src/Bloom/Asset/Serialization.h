#ifndef BLOOM_ASSET_SERIALIZATION_H
#define BLOOM_ASSET_SERIALIZATION_H

#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Yaml.h"

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
        node["AssetType"] = handle.type();
        node["AssetID"] = handle.ID();
        return node;
    }

    static bool decode(Node const& node, bloom::AssetHandle& handle) {
        handle = bloom::AssetHandle(node["AssetType"].as<bloom::AssetType>(),
                                    node["AssetID"].as<utl::uuid>());
        return true;
    }
};

#endif // BLOOM_ASSET_SERIALIZATION_H
