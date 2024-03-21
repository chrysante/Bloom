#ifndef BLOOM_ASSET_ASSETHANDLE_H
#define BLOOM_ASSET_ASSETHANDLE_H

#include <utl/uuid.hpp>

#include "Bloom/Asset/Fwd.h"

namespace bloom {

/// Every asset is uniquely identified by its asset handle
class BLOOM_API AssetHandle {
public:
    /// Construct a null handle
    AssetHandle() = default;

    /// Construct a handle from \p type and \p ID
    AssetHandle(AssetType type, utl::uuid ID): _type(type), _id(ID) {}

    /// \Returns the type of type asset
    AssetType type() const { return _type; }

    /// \Returns the UUID of this asset
    utl::uuid ID() const { return _id; }

    /// Creates a handle with type \p type and a unique ID
    static AssetHandle generate(AssetType type);

    /// \Returns `lhs.ID() == rhs.ID()`
    friend bool operator==(AssetHandle const& lhs, AssetHandle const& rhs) {
        bool const result = lhs.ID() == rhs.ID();
        if (result) {
            assert(lhs.type() == rhs.type() &&
                   "ID is the same but type differs");
        }
        return result;
    }

    /// \Returns `true` if `ID()` is a valid value
    operator bool() const { return !utl::is_null(ID()); }

private:
    AssetType _type = AssetType::Invalid;
    utl::uuid _id;
};

/// Prints \p handle to \p ostream
std::ostream& operator<<(std::ostream& ostream, AssetHandle const& handle);

} // namespace bloom

#endif // BLOOM_ASSET_ASSETHANDLE_H
