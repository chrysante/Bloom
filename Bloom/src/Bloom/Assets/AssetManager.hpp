#pragma once

#include <filesystem>
#include <utl/vector.hpp>
#include <utl/hashmap.hpp>

#include "Bloom/Core/Reference.hpp"
#include "Asset.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "AssetFileHeader.hpp"
#include <future>
#include <optional>
#include <span>

namespace bloom {
	
	class RenderContext;
	class StaticMeshData;
	class StaticMeshAsset;
	class ScriptEngine;
	
	class BLOOM_API AssetManager {
	public:
		virtual ~AssetManager();
		
		/// MARK: Environment
		///
		/// @brief		Set current working directory to \p path
		/// @param path	Must be an absolute directory. Directory will be created if not existent.
		void setWorkingDir(std::filesystem::path path);
		
		void refreshWorkingDir(bool forceOverrides = false);
		
		/// @returns	Absolute path to working directory.
		std::filesystem::path const& workingDir() const { return _workingDir; };
		
		/// @brief	Reload working directory into memory.
//		void refreshFromWorkingDir();
		
		/// @brief	Sets the render context. Required for creating GPU representation of assets such as meshes and materials.
		void setRenderContext(RenderContext* context) { _renderContext = context; }
		/// @brief	Retrieves the render context.
		RenderContext* renderContext() const { return _renderContext; };
		
		
		/// MARK: Create
		///
		/// @brief		Creates a new Asset at \p dest
		/// @param type	Type of the asset.
		/// @param name	Name of the asset.
		/// @param dest	Path to directory relative to current working directory.
		/// @returns	Reference to the created asset.
		Reference<Asset> create(AssetType type, std::string_view name, std::filesystem::path dest);
		
		
		/// MARK: Import
		///
		/// @brief			Imports asset from outside the working directory by reading a file and converting it to an internal representation to be stored on disk.
		/// @param source	Absolute file path to external resource.
		/// @param dest		Path to directory relative to current working directory.
		AssetHandle import(std::filesystem::path source,
						   std::filesystem::path dest);
		
		/// MARK: Remove
		///
		/// @brief 			Deletes asset from memory and from working directory.
		/// @param handle	Handle to asset to be removed.
		void remove(AssetHandle handle);
		
		
		/// MARK: Access
		///
		/// @brief 			Retrieves an asset from an AssetHandle. Does not load the asset into memory. Use makeAvailable to load the asset into memory.
		/// @param handle 	Handle to an Asset. May be null or invalid.
		/// @returns		Reference to the Asset. Will be null if handle was null or invalid.
		Reference<Asset> get(AssetHandle handle);
		
		/// @brief Retrieves the name of an asset.
		/// @param handle 	Handle to an Asset. May be null or invalid.
		/// @returns		Name of the Asset. Will be empty if handle was null or invalid.
		std::string getName(AssetHandle handle) const;
		
		/// @brief Retrieves the filepath of an asset.
		/// @param handle 	Handle to an Asset. May be null or invalid.
		/// @returns		Filepath to the Asset. Will be empty if handle was null or invalid.
		std::filesystem::path getAbsoluteFilepath(AssetHandle handle) const;
		
		/// @brief Retrieves the filepath of an asset.
		/// @param handle 	Handle to an Asset. May be null or invalid.
		/// @returns		Filepath to the Asset. Will be empty if handle was null or invalid.
		std::filesystem::path getRelativeFilepath(AssetHandle handle) const;
		
		/// @brief 			Loads an asset from disk into CPU and/or GPU memory.
		/// @param handle 	Handle to an asset. May be null or invalid.
		/// @param rep		Representation. Shall be any combination of AssetRepresentation::CPU and AssetRepresentation::GPU.
		/// 				Does nothing if specified representation is not compatible with asset type.
		/// @param force	If true, reloads asset into memory even if already available.
		void makeAvailable(AssetHandle handle, AssetRepresentation rep, bool force = false);
		
		/// @brief 		Check if an AssetHandle is valid.
		/// @param 		handle Handle to an asset.
		/// @returns	true iff Handle points to an asset stored in current working directory.
		bool isValid(AssetHandle handle) const;
		
		/// MARK: Save
		/// @brief			Saves asset to disk from current CPU representation.
		/// @param handle	Asset to save.
		void saveToDisk(AssetHandle handle);
		
		
		
		
		/// MARK: Uncategorized
		// path can be relative or absolute
		AssetHandle getHandleFromFile(std::filesystem::path path) const;
		void loadScripts(ScriptEngine&);
		std::span<std::string const> scriptClasses() const { return _scriptClasses; }
		
	private:
		struct InternalAsset {
			WeakReference<Asset> theAsset;
			std::string name;
			/// relative to working directory
			std::filesystem::path diskLocation;
			AssetHandle handle;
		};
		
		Reference<Asset> allocateAsset(AssetHandle) const;
		InternalAsset* find(AssetHandle);
		InternalAsset const* find(AssetHandle) const;
		
		void readAssetMetaData(std::filesystem::path diskLocation, bool forceOverride = false);
		
		/// MARK: Make Available
		void makeStaticMeshAvailable(InternalAsset&, AssetRepresentation rep, bool force);
		void makeMaterialAvailable(InternalAsset&, AssetRepresentation rep, bool force);
		void makeSceneAvailable(InternalAsset&, AssetRepresentation rep, bool force);
		void makeScriptAvailable(InternalAsset&, AssetRepresentation rep, bool force);
		
		
		///MARK: Disk -> Memory
		Reference<StaticMeshData> readStaticMeshFromDisk(std::filesystem::path source);
		Scene loadSceneFromDisk(std::filesystem::path source);
		std::string loadTextFromDisk(std::filesystem::path source);
		
		/// MARK: Memory -> GPU
		void loadStaticRenderMesh(InternalAsset&);
		
		/// MARK: Memory -> Disk
		void flushToDisk(AssetHandle);
		void flushStaticMeshToDisk(AssetHandle);
		void flushMaterialToDisk(AssetHandle);
		void flushSceneToDisk(AssetHandle);
		void flushScriptToDisk(AssetHandle);
		
		
		/// MARK: Import
		AssetType getImportType(std::string_view extension) const;
		AssetHandle store(Reference<Asset> asset, std::string_view name, std::filesystem::path dest);
		void importStaticMesh(StaticMeshAsset* meshOut, std::filesystem::path source) const;
		
		
		/// MARK: File Handling
		[[ nodiscard ]] std::filesystem::path makeRelative(std::filesystem::path const&) const;
		[[ nodiscard ]] std::filesystem::path makeAbsolute(std::filesystem::path const&) const;
		AssetFileHeader readHeader(std::filesystem::path) const;
		AssetFileHeader readHeader(std::fstream&) const;
		void handleFileError(std::fstream&, std::filesystem::path const&) const;
		
		
	private:
		utl::hashmap<utl::UUID, InternalAsset> assets;
		
		RenderContext* _renderContext = nullptr;
		std::filesystem::path _workingDir;
		utl::vector<std::string> _scriptClasses;
	};

	
	
	
	
}
