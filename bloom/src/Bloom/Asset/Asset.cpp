#include "Bloom/Asset/Asset.hpp"

#include <utl/math.hpp>

#include "Bloom/Core/Debug.hpp"

using namespace bloom;

FileExtension bloom::toExtension(std::filesystem::path const& path) {
    return toExtension(std::string_view(path.extension().string()));
}

FileExtension bloom::toExtension(std::string_view extension) {
    if (extension == ".bmesh") {
        return FileExtension::bmesh;
    }
    if (extension == ".bmat") {
        return FileExtension::bmat;
    }
    if (extension == ".bmatinst") {
        return FileExtension::bmatinst;
    }
    if (extension == ".bscene") {
        return FileExtension::bscene;
    }
    if (extension == ".sc") {
        return FileExtension::chai;
    }
    return FileExtension::invalid;
}

bool bloom::hasHeader(FileExtension ext) { return ext != FileExtension::chai; }

utl::uuid bloom::toUUID(std::string_view str) {
    char const* const begin = str.data();
    std::size_t const offset = str.size() / 2;
    struct {
        std::size_t first, second;
    } values = { utl::hash_string(std::string_view(begin, offset)),
                 utl::hash_string(
                     std::string_view(begin + offset, str.size() - offset)) };
    return utl::uuid::construct_from_value(
        utl::bit_cast<utl::uuid::value_type>(values));
}

/// MARK: - AssetType
BLOOM_API std::string bloom::toString(AssetType type) {
    auto const i = utl::to_underlying(type);
    assert(std::popcount(i) <= 1 && "'type' is a Mask");
    if (type == AssetType::none) {
        return "None";
    }
    return std::array{
        "Static Mesh",       "Skeletal Mesh", "Material",
        "Material Instance", "Scene",         "Script"
    }[utl::log2(i)];
}

BLOOM_API AssetType bloom::assetTypeFromString(std::string_view str) {
    if (str == "Static Mesh") {
        return AssetType::staticMesh;
    }
    if (str == "Skeletal Mesh") {
        return AssetType::skeletalMesh;
    }
    if (str == "Material") {
        return AssetType::material;
    }
    if (str == "Material Instance") {
        return AssetType::materialInstance;
    }
    if (str == "Scene") {
        return AssetType::scene;
    }
    return AssetType::none;
}

BLOOM_API std::ostream& bloom::operator<<(std::ostream& str, AssetType type) {
    return str << toString(type);
}

BLOOM_API std::string bloom::toExtension(AssetType type) {
    auto const i = utl::to_underlying(type);
    assert(std::popcount(i) <= 1 && "'type' is a Mask");
    if (type == AssetType::none) {
        return std::string{};
    }
    return std::array{
        ".bmesh", ".invalid", ".bmat", ".bmatinst", ".bscene", ".chai",
    }[utl::log2(i)];
}
BLOOM_API AssetType bloom::toAssetType(FileExtension ext) {
    switch (ext) {
    case FileExtension::bmesh:
        return AssetType::staticMesh;

    case FileExtension::bmat:
        return AssetType::material;

    case FileExtension::bmatinst:
        return AssetType::materialInstance;

    case FileExtension::bscene:
        return AssetType::scene;

    case FileExtension::chai:
        return AssetType::script;

    default:
        return AssetType::none;
    }
}

BLOOM_API FileFormat bloom::toFileFormat(FileExtension ext) {
    switch (ext) {
    case FileExtension::bmesh:
        return FileFormat::binary;

    case FileExtension::bmat:
        return FileFormat::binary;

    case FileExtension::bmatinst:
        return FileFormat::text;

    case FileExtension::bscene:
        return FileFormat::text;

    case FileExtension::chai:
        return FileFormat::text;

    default:
        return FileFormat::text;
    }
}

/// MARK: - AssetHandle
AssetHandle AssetHandle::generate(AssetType type) {
    return AssetHandle(type, utl::uuid::generate());
}

bool bloom::operator==(AssetHandle const& a, AssetHandle const& b) {
    bool const result = a.id() == b.id();
    assert(!result ||
           a.type() == b.type() && "ID is the same but type differs");
    return result;
}

AssetHandle::operator bool() const { return !utl::is_null(id()); }
