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

/// Forward declaration of all asset types
#define BLOOM_ASSET_TYPE_DEF(Name, ...) class Name;
#include "Bloom/Asset/Assets.def"

/// List of all asset types
enum class AssetType : unsigned {
#define BLOOM_ASSET_TYPE_DEF(Name, ...) Name,
#include "Bloom/Asset/Assets.def"
    COUNT = Invalid
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

/// Constructs a UUID by hashing \p text
/// TODO: This should be private and ideally should not exist
utl::uuid toUUID(std::string_view text);

/// \Returns a string representation of asset type \p type
BLOOM_API std::string toString(AssetType type);

/// Prints \p type to \p ostream
BLOOM_API std::ostream& operator<<(std::ostream& ostream, AssetType type);

/// Bit set of different asset representations
enum class AssetRepresentation { None = 0, CPU = 1 << 0, GPU = 1 << 1 };

/// Enable bitwise operators for `AssetRepresentation`
UTL_ENUM_OPERATORS(AssetRepresentation);

} // namespace bloom

#endif // BLOOM_ASSET_FWD_H
