#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

#include <filesystem>
#include <iosfwd>
#include <utl/common.hpp>
#include <utl/uuid.hpp>
#include <yaml-cpp/helpers.hpp>
#include <yaml-cpp/yaml.h>

namespace bloom {

enum class FileFormat { binary, text };

/// MARK: - AssetType
enum class AssetType : unsigned {
    none = 0,

    staticMesh   = 1 << 0,
    skeletalMesh = 1 << 1, // not supported yet...
    mesh         = staticMesh | skeletalMesh,

    material         = 1 << 2,
    materialInstance = 1 << 3,

    scene = 1 << 4,

    script = 1 << 5,

    itemCount = 6
};
UTL_ENUM_OPERATORS(AssetType);

enum class FileExtension { invalid = 0, bmesh, bmat, bmatinst, bscene, chai };

FileExtension toExtension(std::filesystem::path const&);
FileExtension toExtension(std::string_view);

bool hasHeader(FileExtension);

utl::uuid toUUID(std::string_view);

BLOOM_API std::string toString(AssetType);
BLOOM_API AssetType assetTypeFromString(std::string_view);
BLOOM_API std::ostream& operator<<(std::ostream&, AssetType);

BLOOM_API std::string toExtension(AssetType);
BLOOM_API AssetType toAssetType(FileExtension);
BLOOM_API FileFormat toFileFormat(FileExtension);

/// MARK: - AssetRepresentation
enum class AssetRepresentation { CPU = 1 << 0, GPU = 1 << 1 };
UTL_ENUM_OPERATORS(AssetRepresentation);

/// MARK: - AssetHandle
class BLOOM_API AssetHandle {
public:
    AssetHandle() = default;
    AssetHandle(AssetType type, utl::uuid id): _type(type), _id(id) {}

    AssetType type() const { return _type; }
    utl::uuid id() const { return _id; }

    static AssetHandle generate(AssetType);

    friend bool operator==(AssetHandle const&, AssetHandle const&);
    operator bool() const;

private:
    AssetType _type = AssetType::none;
    utl::uuid _id;
};

/// MARK: - Asset
/// Base Class for all assets
class BLOOM_API Asset {
public:
    explicit Asset(AssetHandle handle, std::string name):
        mHandle(handle), mName(std::move(name)) {}

    virtual ~Asset() = default;

    AssetHandle handle() const { return mHandle; }
    std::string_view name() const { return mName; }

private:
    AssetHandle mHandle;
    std::string mName;
};

class StaticMesh;
class Material;
class MaterialInstance;
class Scene;
class Script;

auto dispatchAssetType(AssetType type, auto&& f) {
    switch (type) {
    case AssetType::staticMesh:
        return f(utl::tag<StaticMesh>{});
    case AssetType::material:
        return f(utl::tag<Material>{});
    case AssetType::materialInstance:
        return f(utl::tag<MaterialInstance>{});
    case AssetType::scene:
        return f(utl::tag<Scene>{});
    case AssetType::script:
        return f(utl::tag<Script>{});

    default:
        BL_DEBUGBREAK();
        std::terminate();
    }
}

} // namespace bloom

template <>
struct YAML::convert<bloom::AssetHandle> {
    static Node encode(bloom::AssetHandle const& h) {
        Node node;
        node["Type"] = std::string(toString(h.type()));
        node["ID"]   = h.id();
        return node;
    }

    static bool decode(Node const& node, bloom::AssetHandle& h) {
        auto const type =
            bloom::assetTypeFromString(node["Type"].as<std::string>());
        auto const id = node["ID"].as<utl::uuid>();
        h             = bloom::AssetHandle(type, id);
        return true;
    }
};
