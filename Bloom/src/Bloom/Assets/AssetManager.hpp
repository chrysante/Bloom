#pragma once

#include <filesystem>
#include <utl/vector.hpp>
#include <utl/hashmap.hpp>

#include "Bloom/Core/Reference.hpp"
#include "Asset.hpp"
#include "AssetFileHeader.hpp"
#include <future>
#include <optional>

namespace bloom {
	
	class RenderContext;
	class StaticMeshData;
	class StaticMeshAsset;
	
	BLOOM_API std::string_view extension(AssetType);
	
	class BLOOM_API AssetManager {
	public:
		virtual ~AssetManager();
		
		void setWorkingDir(std::filesystem::path);
		void refreshFromWorkingDir();
		std::filesystem::path const& workingDir() const { return _workingDir; };
		
		void setRenderContext(RenderContext* context) { _renderContext = context; }
		RenderContext* renderContext() const { return _renderContext; };
		
		Reference<Asset> getAsset(AssetHandle);
		Asset* getAssetWeak(AssetHandle);
		std::string getName(AssetHandle);
		
		void makeAvailable(AssetHandle, AssetRepresentation, bool force = false);
		
//		std::optional<std::future<Reference<Asset>>>
//			requestAsset(AssetHandle, AssetRepresentation);
		
	public:
		// this api is only for the asset browser. how to expose this only there with befriending it?
		AssetHandle getHandleFromFile(std::filesystem::path);
		
	protected:
		void makeStaticMeshAvailable(Asset*, AssetRepresentation, bool force);
		void loadStaticRenderMesh(StaticMeshAsset*);
		
		Reference<Asset> createReference(AssetHandle, std::string_view name,
										 std::filesystem::path const& diskLocation);
		bool storeReference(Reference<Asset>);
		AssetFileHeader readHeader(std::filesystem::path);
		AssetFileHeader readHeader(std::fstream&);
		
		void flushToDisk(AssetHandle, std::string_view name,
						 StaticMeshData const&, std::filesystem::path dest);
		Reference<StaticMeshData> readStaticMeshFromDisk(std::filesystem::path source);
		
		[[ nodiscard ]] std::filesystem::path makeRelative(std::filesystem::path const&) const;
		[[ nodiscard ]] std::filesystem::path makeAbsolute(std::filesystem::path const&) const;
		void handleFileError(std::fstream&, std::filesystem::path const&) const;
		
	protected:
		utl::hashmap<utl::UUID, Reference<Asset>> assets;
	
	private:
		RenderContext* _renderContext = nullptr;
		std::filesystem::path _workingDir;
	};
	
}
