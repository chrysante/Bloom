#include "Bloom/Asset/Fwd.hpp"

#include "Bloom/Core/Debug.hpp"

using namespace bloom;

std::string bloom::toString(FileExtension ext) {
    switch (ext) {
    case FileExtension::Bmesh:
        return "bmesh";
    case FileExtension::Bmat:
        return "bmat";
    case FileExtension::Bmatinst:
        return "bmatinst";
    case FileExtension::Bscene:
        return "bscene";
    case FileExtension::Scatha:
        return "sc";
    }
}

std::filesystem::path bloom::append(std::filesystem::path filename,
                                    FileExtension ext) {
    filename += "." + toString(ext);
    return filename;
}

std::optional<FileExtension> bloom::toExtension(
    std::filesystem::path const& path) {
    return toExtension(std::string_view(path.extension().string()));
}

std::optional<FileExtension> bloom::toExtension(std::string_view extension) {
    if (extension == ".bmesh") {
        return FileExtension::Bmesh;
    }
    if (extension == ".bmat") {
        return FileExtension::Bmat;
    }
    if (extension == ".bmatinst") {
        return FileExtension::Bmatinst;
    }
    if (extension == ".bscene") {
        return FileExtension::Bscene;
    }
    if (extension == ".sc") {
        return FileExtension::Scatha;
    }
    return std::nullopt;
}

bool bloom::hasHeader(FileExtension ext) {
    return ext != FileExtension::Scatha;
}

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

std::string bloom::toString(AssetType type) {
    switch (type) {
#define BLOOM_ASSET_TYPE_DEF(Name, ...)                                        \
    case AssetType::Name:                                                      \
        return #Name;
#include "Bloom/Asset/Assets.def"
    }
    return "InvalidAsset";
}

std::ostream& bloom::operator<<(std::ostream& str, AssetType type) {
    return str << toString(type);
}

FileExtension bloom::toExtension(AssetType type) {
    switch (type) {
    case AssetType::Asset:
        BL_UNREACHABLE();
    case AssetType::Mesh:
        BL_UNREACHABLE();
    case AssetType::StaticMesh:
        return FileExtension::Bmesh;
    case AssetType::SkeletalMesh:
        BL_UNREACHABLE();
    case AssetType::Material:
        return FileExtension::Bmat;
    case AssetType::MaterialInstance:
        return FileExtension::Bmatinst;
    case AssetType::Scene:
        return FileExtension::Bscene;
    case AssetType::ScriptSource:
        return FileExtension::Scatha;
    }
}

AssetType bloom::toAssetType(FileExtension ext) {
    switch (ext) {
    case FileExtension::Bmesh:
        return AssetType::StaticMesh;
    case FileExtension::Bmat:
        return AssetType::Material;
    case FileExtension::Bmatinst:
        return AssetType::MaterialInstance;
    case FileExtension::Bscene:
        return AssetType::Scene;
    case FileExtension::Scatha:
        return AssetType::ScriptSource;
    }
}

FileFormat bloom::toFileFormat(FileExtension ext) {
    switch (ext) {
    case FileExtension::Bmesh:
        return FileFormat::Binary;
    case FileExtension::Bmat:
        return FileFormat::Binary;
    case FileExtension::Bmatinst:
        return FileFormat::Text;
    case FileExtension::Bscene:
        return FileFormat::Text;
    case FileExtension::Scatha:
        return FileFormat::Text;
    }
}
