#ifndef BLOOM_ASSET_ASSET_H
#define BLOOM_ASSET_ASSET_H

#include <concepts>
#include <filesystem>
#include <iosfwd>

#include <utl/common.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Reference.h"

/// Defines the constructor for the derived asset type \p Type
#define BL_DEFINE_ASSET_CTOR(Type, Parent)                                     \
    explicit Type(AssetHandle handle, std::string name):                       \
        Parent(handle, std::move(name)) {                                      \
        BL_EXPECT(handle.type() == AssetType::Type, "Mismatched asset type");  \
    }

namespace bloom {

/// Base Class of all assets
class BLOOM_API Asset {
public:
    Asset(Asset const&) = delete;
    Asset& operator=(Asset const&) = delete;

    /// \Returns the handle of this asset
    AssetHandle handle() const { return mHandle; }

    /// \Returns the name of this asset
    std::string const& name() const { return mName; }

protected:
    explicit Asset(AssetHandle handle, std::string name):
        mHandle(handle), mName(std::move(name)) {}

private:
    friend class AssetManager;

    AssetHandle mHandle;
    std::string mName;
};

/// For `dyncast` to work
inline AssetType dyncast_get_type(std::derived_from<Asset> auto const& asset) {
    return asset.handle().type();
}

/// Asset manager events

/// This message instructs the asset manager to retrieve the asset with
/// \p handle and assign it to \p asset
///
/// The representation \p makeAvailable is made available
///
/// \p diskLocation will be an absolute path (if set)
struct BLOOM_API AMGetAsset {
    AssetHandle handle;
    Reference<Asset>* asset = nullptr;
    AssetRepresentation representation = AssetRepresentation::None;
    std::filesystem::path* diskLocation = nullptr;
};

/// Dispatches to the asset manager to get the asset with handle \p handle
Reference<Asset> getAsset(AssetHandle handle,
                          AssetRepresentation repr = AssetRepresentation::None);

/// \overload
template <std::derived_from<Asset> AssetType>
Reference<AssetType> getAsset(
    AssetHandle handle, AssetRepresentation repr = AssetRepresentation::None) {
    return as<AssetType>(getAsset(handle, repr));
}

/// Represents a script source file
/// TODO: Move this to its own file
class ScriptSource: public Asset {
public:
    BL_DEFINE_ASSET_CTOR(ScriptSource, Asset)
};

} // namespace bloom

#endif // BLOOM_ASSET_ASSET_H
