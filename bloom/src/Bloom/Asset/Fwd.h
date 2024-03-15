#ifndef BLOOM_ASSET_FWD_H
#define BLOOM_ASSET_FWD_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <utl/common.hpp>
#include <utl/dyncast.hpp>
#include <utl/uuid.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Dyncast.h"

namespace bloom {

/// List of possible file formats
enum class FileFormat { Binary, Text };

/// Forward declaration of all asset types
#define BLOOM_ASSET_TYPE_DEF(Name, ...) class Name;
#include "Bloom/Asset/Assets.def"

/// List of all asset types
enum class AssetType : unsigned {
#define BLOOM_ASSET_TYPE_DEF(Name, ...) Name,
#include "Bloom/Asset/Assets.def"
    LAST = ScriptSource
};

///
inline constexpr AssetType InvalidAssetType{
    utl::to_underlying(AssetType::LAST) + 1
};

/// \Returns `std::invoke(callback, utl::tag</* ID as type */>{})`
decltype(auto) dispatchAssetType(AssetType ID, auto&& callback) {
    switch (ID) {
#define BLOOM_ASSET_TYPE_DEF(Name, Parent, C)                                  \
    case AssetType::Name:                                                      \
        if constexpr (utl::dc::Corporeality::C ==                              \
                      utl::dc::Corporeality::Concrete)                         \
        {                                                                      \
            return std::invoke(callback, utl::tag<Name>{});                    \
        }                                                                      \
        BL_UNREACHABLE();
#include "Bloom/Asset/Assets.def"
    }
    BL_UNREACHABLE();
}

/// To make the base parent case in the dyncast macro work
using VoidParent = void;

} // namespace bloom

/// Map types to enum values
#define BLOOM_ASSET_TYPE_DEF(Type, Parent, Corporeality)                       \
    UTL_DYNCAST_DEFINE(bloom::Type, bloom::AssetType::Type, bloom::Parent,     \
                       Corporeality)
#include "Bloom/Asset/Assets.def"

namespace bloom {

/// List of possible asset file extensions
enum class FileExtension { Bmesh, Bmat, Bmatinst, Bscene, Scatha };

///
std::string toString(FileExtension ext);

///
std::filesystem::path append(std::filesystem::path filename, FileExtension ext);

/// Converts \p ext to `FileExtension`
std::optional<FileExtension> toExtension(std::filesystem::path const& ext);

/// \overload for `std::string_view`
std::optional<FileExtension> toExtension(std::string_view ext);

/// TODO: Should this be private?
bool hasHeader(FileExtension ext);

/// Constructs a UUID by hashing \p text
/// TODO: This should be private and ideally should not exist
utl::uuid toUUID(std::string_view text);

/// \Returns a string representation of asset type \p type
BLOOM_API std::string toString(AssetType type);

/// Prints \p type to \p ostream
BLOOM_API std::ostream& operator<<(std::ostream& ostream, AssetType type);

/// \Returns the file extension corresponding to \p type
/// TODO: Make this a member function of `Asset`
BLOOM_API FileExtension toExtension(AssetType type);

/// \Returns the asset type corresponding to the file extension \p ext
BLOOM_API AssetType toAssetType(FileExtension ext);

/// \Returns the file format corresponding to the extension \p ext
BLOOM_API FileFormat toFileFormat(FileExtension ext);

/// Bit set of different asset representations
enum class AssetRepresentation { None = 0, CPU = 1 << 0, GPU = 1 << 1 };

/// Enable bitwise operators for `AssetRepresentation`
UTL_ENUM_OPERATORS(AssetRepresentation);

} // namespace bloom

#endif // BLOOM_ASSET_FWD_H
