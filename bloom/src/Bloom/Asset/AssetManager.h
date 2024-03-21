#ifndef BLOOM_ASSET_ASSETMANAGER_H
#define BLOOM_ASSET_ASSETMANAGER_H

#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <span>

#include <utl/hashmap.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Core.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Scene/Scene.h"

namespace bloom {

class HardwareDevice;
struct StaticMeshData;

/// Manages lifetime and representation of all assets.
/// Assets can be loaded from disk to CPU or GPU memory and vice versa
class BLOOM_API AssetManager: public CoreSystem {
public:
    /// Construct an empty asset manager. Does not do anything except for
    /// setting up class invariants.
    AssetManager();

    ///
    ~AssetManager();

    /// Opens the project at \p path
    ///
    /// \param path	Must be an absolute directory. Directory will be created if
    /// not existent.
    void openProject(std::filesystem::path path);

    /// Scans the entire project directory for asset files and creates registry
    /// entries for unregistered files. Creates a registry if none exists yet
    ///
    /// In the future we may check if asset source files have been updated and
    /// reimport assets
    void refreshProject();

    /// \Returns the absolute path to the current project directory.
    std::filesystem::path projectRootDir() const;

    /// \Returns the device.
    HardwareDevice& device() const;

    /// Creates a new asset in project subdirectory \p destDir
    ///
    /// \Param type	The type of the asset.
    /// \Param name	The name of the asset. The asset location will be `destDir /
    /// name.{type-extension}` \Param dest	Path to directory relative to the
    /// project directory. \Returns a reference to the created asset.
    Reference<Asset> createAsset(AssetType type, std::string name,
                                 std::filesystem::path destDir);

    /// Imports asset from outside the working directory by reading a file and
    /// converting it to an internal representation to be stored on disk.
    ///
    /// \Param name Name of the asset to create
    /// \Param source Absolute file path to external resource
    /// \Param dest Path to directory relative to current working
    /// directory.
    AssetHandle importAsset(std::string name, std::filesystem::path source,
                            std::filesystem::path dest);

    /// Renames the asset \p handle to \p name
    ///
    /// Updates the registry and asset filename
    void renameAsset(AssetHandle handle, std::string name);

    /// Deletes asset from memory and from working directory
    ///
    /// \Param handle Handle to the asset to be deleted
    void deleteAsset(AssetHandle handle);

    /// Retrieves an asset from an AssetHandle. Does not load the asset into
    /// memory. Use `makeAvailable()` to load the asset into memory.
    ///
    /// \Param handle Handle to an Asset. May be null or invalid, in which case
    /// we return null.
    ///
    /// \Returns Reference to the Asset or null if handle was null or invalid.
    Reference<Asset> get(AssetHandle handle);

    /// \Returns the name of the asset handle \p handle or an empty string if
    /// the asset does not exist
    std::string getName(AssetHandle handle) const;

    /// Retrieves the absolute filepath of the asset \p handle
    ///
    /// \Param handle Handle to an Asset. May be null or invalid.
    ///
    /// \Returns the filepath of the asset referred to by \p handle. Will be
    /// empty if \p handle was null or invalid.
    std::filesystem::path getAbsoluteFilepath(AssetHandle handle) const;

    /// Retrieves the filepath of the asset \p handle relative to the current
    /// working directory
    ///
    /// \Param handle Handle to an asset. May be null or invalid.
    ///
    /// \Returns the filepath of the asset \p handle. Will be empty if \p handle
    /// was null or invalid.
    std::filesystem::path getRelativeFilepath(AssetHandle handle) const;

    /// Loads an asset from disk into CPU and/or GPU memory.
    ///
    /// Does nothing if specified representation is not compatible with asset
    /// type.
    ///
    /// \Param handle Handle to an asset. May be null or invalid.
    ///
    /// \Param repr Representation. Shall be any combination of
    /// `AssetRepresentation::CPU` and `AssetRepresentation::GPU`.
    ///
    /// \Param forceReload If true, reloads asset into memory even if already
    /// available.
    ///
    /// \Returns `true` if operation was successful.
    ///
    bool makeAvailable(AssetHandle handle, AssetRepresentation repr,
                       bool forceReload = false);

    /// Check if an AssetHandle is valid, i.e. refers to an asset in this
    /// manager. \Param handle Handle to an asset. \Returns true if Handle
    /// points to an asset stored in current working directory.
    bool isValid(AssetHandle handle) const;

    /// Saves asset to disk from current CPU representation.
    /// \Param handle Asset to save.
    void saveToDisk(AssetHandle handle);

    ///
    void saveAll();

    /// TODO: Delete this
    /// We use this method because in the directory view we iterate over the
    /// filesystem, however we should iterate over the asset manager Path can be
    /// relative or absolute
    AssetHandle getHandleFromFile(std::filesystem::path path) const;

    ///
    void compileScripts();

    /// Public to make implementation easier
    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_ASSET_ASSETMANAGER_H
