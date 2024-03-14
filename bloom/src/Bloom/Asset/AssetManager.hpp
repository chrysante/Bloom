#ifndef BLOOM_ASSET_ASSETMANAGER_H
#define BLOOM_ASSET_ASSETMANAGER_H

#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <span>

#include <utl/hashmap.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application/CoreSystem.hpp"
#include "Bloom/Asset/Asset.hpp"
#include "Bloom/Asset/AssetFileHeader.hpp"
#include "Bloom/Core/Core.hpp"
#include "Bloom/Graphics/Material/Material.hpp"
#include "Bloom/Graphics/Material/MaterialInstance.hpp"
#include "Bloom/Scene/Scene.hpp"

namespace bloom {

class HardwareDevice;
struct StaticMeshData;

/// Manages lifetime and representation of all assets.
/// Assets can be loaded from disk to CPU or GPU memory and vice versa
class BLOOM_API AssetManager: public CoreSystem {
public:
    /// Construct an empty asset manager
    AssetManager();

    ///
    ~AssetManager();

    /// MARK: Environment
    ///
    /// @brief		Set current working directory to \p path
    /// @param path	Must be an absolute directory. Directory will be created if
    /// not existent.
    void setWorkingDir(std::filesystem::path path);

    /// ???
    void refreshWorkingDir(bool forceOverrides = false);

    /// @returns	Absolute path to working directory.
    std::filesystem::path workingDir() const;

    /// @brief	Retrieves the device.
    HardwareDevice& device() const;

    /// MARK: Create
    ///
    /// @brief		Creates a new Asset at \p dest
    /// @param type	Type of the asset.
    /// @param name	Name of the asset.
    /// @param dest	Path to directory relative to current working directory.
    /// @returns	Reference to the created asset.
    Reference<Asset> create(AssetType type, std::string name,
                            std::filesystem::path destDir);

    /// MARK: Import
    ///
    /// @brief			Imports asset from outside the working directory by
    /// reading a file and converting it to an internal representation to be
    /// stored on disk.
    /// @param source	Absolute file path to external resource.
    /// @param dest		Path to directory relative to current working
    /// directory.
    AssetHandle import(std::filesystem::path source,
                       std::filesystem::path dest);

    /// MARK: Remove
    ///
    /// @brief 			Deletes asset from memory and from working
    /// directory.
    /// @param handle	Handle to asset to be removed.
    void remove(AssetHandle handle);

    /// MARK: Access
    ///
    /// @brief 			Retrieves an asset from an AssetHandle. Does not load
    /// the asset into memory. Use makeAvailable to load the asset into memory.
    /// @param handle 	Handle to an Asset. May be null or invalid.
    /// @returns		Reference to the Asset. Will be null if handle was null
    /// or invalid.
    Reference<Asset> get(AssetHandle handle);

    /// @brief Retrieves the name of an asset.
    /// @param handle 	Handle to an Asset. May be null or invalid.
    /// @returns		Name of the Asset. Will be empty if handle was null or
    /// invalid.
    std::string getName(AssetHandle handle) const;

    /// Retrieves the absolute filepath of the asset \p handle
    ///
    /// \param handle Handle to an Asset. May be null or invalid.
    ///
    /// \Returns the filepath of the asset referred to by \p handle. Will be
    /// empty if \p handle was null or invalid.
    std::filesystem::path getAbsoluteFilepath(AssetHandle handle) const;

    /// Retrieves the filepath of the asset \p handle relative to the current
    /// working directory
    ///
    /// \param handle Handle to an asset. May be null or invalid.
    ///
    /// \Returns the filepath of the asset \p handle. Will be empty if \p handle
    /// was null or invalid.
    std::filesystem::path getRelativeFilepath(AssetHandle handle) const;

    /// Loads an asset from disk into CPU and/or GPU memory.
    ///
    /// Does nothing if specified representation is not compatible with asset
    /// type.
    ///
    /// \Returns `true` if operation was successful.
    ///
    /// \param handle Handle to an asset. May be null or invalid.
    ///
    /// \param rep Representation. Shall be any combination of
    /// `AssetRepresentation::CPU` and `AssetRepresentation::GPU`.

    /// \param force If true, reloads asset into memory even if already
    /// available.
    bool makeAvailable(AssetHandle handle, AssetRepresentation rep,
                       bool force = false);

    /// @brief 		Check if an AssetHandle is valid.
    /// @param 		handle Handle to an asset.
    /// @returns	true iff Handle points to an asset stored in current working
    /// directory.
    bool isValid(AssetHandle handle) const;

    /// MARK: Save
    /// @brief			Saves asset to disk from current CPU
    /// representation.
    /// @param handle	Asset to save.
    void saveToDisk(AssetHandle handle);

    void saveAll();

    /// MARK: Uncategorized
    // path can be relative or absolute
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
