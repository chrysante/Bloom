#include "AssetManager.hpp"

#include "Bloom/Core/Debug.hpp"

#include "StaticMeshAsset.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Graphics/StaticRenderMesh.hpp"

#include <fstream>
#include <utl/filesystem_ext.hpp>

namespace bloom {
	
	BLOOM_API std::string_view extension(AssetType type) {
		switch (type) {
			case AssetType::staticMesh:
				return ".bmesh";
				break;
			case AssetType::skinnedMesh:
				return ".bskinnedmesh";
				break;
			case AssetType::material:
				return ".bmaterial";
				break;
				
			default:
				return ".blob";
				break;
		}
	}
	
	AssetManager::~AssetManager() = default;
	
	void AssetManager::setWorkingDir(std::filesystem::path path) {
		_workingDir = path;
		refreshFromWorkingDir();
	}
	
	void AssetManager::refreshFromWorkingDir() {
		assets.clear();
		if (workingDir().empty()) {
			return;
		}
		auto dirItr = std::filesystem::recursive_directory_iterator(workingDir());
		for (auto const& entry: dirItr) {
			if (!std::filesystem::is_regular_file(entry.path())) {
				continue;
			}
			if (utl::is_hidden(entry.path())) {
				continue;
			}
			auto const header = readHeader(entry.path());
			auto asset = createReference(header.getAssetHandle(),
										 header.name(),
										 makeRelative(entry.path()));
			if (!storeReference(std::move(asset))) {
				bloomLog(warning,
						 "Failed to store Reference. Asset '{}' with ID {} "
						 "already exists,",
						 header.name(), header.id);
			}
			
		}
	}

	Reference<Asset> AssetManager::createReference(AssetHandle handle, std::string_view name,
												   std::filesystem::path const& diskLocation)
	{
		switch (handle.type()) {
			case AssetType::staticMesh:
				return allocateRef<StaticMeshAsset>(handle, name, diskLocation);
				break;
				
			default:
				bloomDebugbreak("Unimplemented");
				std::terminate();
				break;
		}
	}
		
	bool AssetManager::storeReference(Reference<Asset> asset) {
		auto [itr, success] = assets.insert({ asset->id(), std::move(asset) });
		return success;
	}
	
	Reference<Asset> AssetManager::getAsset(AssetHandle handle) {
		auto const itr = assets.find(handle.id());
		if (itr == assets.end()) {
			bloomLog(error, "Failed to get Asset");
			return nullptr;
		}
		
		auto& asset = itr->second;
		bloomAssert(handle.id() == asset->id());
		bloomAssert(handle.type() == asset->type());
		return asset;
	}
	
	Asset* AssetManager::getAssetWeak(AssetHandle handle) {
#warning make this implementation more efficient
		return getAsset(handle).get();
	}
		
	std::string AssetManager::getName(AssetHandle handle) {
		auto* asset = getAssetWeak(handle);
		return asset ? std::string(asset->name()) : std::string{};
	}
	
	void AssetManager::makeAvailable(AssetHandle handle, AssetRepresentation rep, bool force) {
		auto const itr = assets.find(handle.id());
		if (itr == assets.end()) {
			bloomLog(error, "Failed to make available: ID not found [ID = {}]", handle.id());
			return;
		}
		
		auto const assetRef = itr->second;
		bloomAssert(handle.id() == assetRef->id());
		bloomAssert(handle.type() == assetRef->type());
		
		switch (handle.type()) {
			case AssetType::staticMesh: {
				makeStaticMeshAvailable(utl::down_cast<StaticMeshAsset*>(assetRef.get()), rep, force);
				break;
			}
				
			default:
				bloomDebugbreak("Unimplemented");
				break;
		}
	}
	
//	std::optional<std::future<Reference<Asset>>>
//	AssetManager::requestAsset(AssetHandle handle, AssetRepresentation rep)
//	{
//		auto const itr = assets.find(handle.id());
//		if (itr == assets.end()) {
//			bloomLog(error, "Failed to fullfill Asset Request: ID not found [ID = {}]", handle.id());
//			return std::nullopt;
//		}
//
//		auto const assetRef = itr->second;
//		bloomAssert(handle.id() == assetRef->id());
//		bloomAssert(handle.type() == assetRef->type());
//
//		// use this seemingly unnecessary approach to future proof for deferred asset streaming
//		std::promise<Reference<Asset>> result;
//
//		switch (handle.type()) {
//			case AssetType::staticMesh: {
//				loadStaticMeshAsset(utl::down_cast<StaticMeshAsset*>(assetRef.get()), rep);
//				break;
//			}
//
//			default:
//				bloomDebugbreak("Unimplemented");
//				break;
//		}
//
//		result.set_value(std::move(assetRef));
//		return result.get_future();
//	}
	
	void AssetManager::makeStaticMeshAvailable(Asset* a, AssetRepresentation rep, bool force) {
		auto* const asset = utl::down_cast<StaticMeshAsset*>(a);
		
		if (test(rep & AssetRepresentation::CPU) && (!asset->_meshData || force)) {
			asset->_meshData = readStaticMeshFromDisk(asset->diskLocation());
		}
		
		if (test(rep & AssetRepresentation::GPU) && (!asset->_renderMesh || force)) {
			loadStaticRenderMesh(asset);
		}
	}
	
	void AssetManager::loadStaticRenderMesh(StaticMeshAsset* asset) {
		if (asset->_renderMesh) {
			return;
		}
		
		Reference<StaticMeshData> smData = asset->_meshData;
		if (!smData) {
			smData = readStaticMeshFromDisk(asset->diskLocation());
		}
		
		asset->_renderMesh = allocateRef<StaticRenderMesh>();
		auto* const mesh = asset->_renderMesh.get();
		
		mesh->_handle = asset->handle();
		mesh->vertexBuffer = renderContext()->createVertexBuffer(smData->vertices.data(),
																 smData->vertices.size() * sizeof(smData->vertices[0]));
		mesh->indexBuffer = renderContext()->createIndexBuffer(smData->indices);
	}
	
	AssetHandle AssetManager::getHandleFromFile(std::filesystem::path path) {
		std::fstream file(workingDir() / path, std::ios::in | std::ios::binary);
		if (!file) {
			bloomLog(error, "Failed to read file from working directory");
			return {};
		}
		
		auto const header = readHeader(file);
		return AssetHandle(header);
	}
	
	AssetFileHeader AssetManager::readHeader(std::filesystem::path path) {
		path = makeAbsolute(path);
		std::fstream file(path);
		handleFileError(file, path);
		return readHeader(file);
	}
	
	AssetFileHeader AssetManager::readHeader(std::fstream& file) {
		AssetFileHeader header;
		file.read((char*)&header, sizeof(AssetFileHeader));
		return header;
	}
	
	void AssetManager::flushToDisk(AssetHandle handle,
								   std::string_view name,
								   StaticMeshData const& mesh,
								   std::filesystem::path dest)
	{
		std::uint64_t const vertexDataSize = mesh.vertices.size() * sizeof(bloom::Vertex3D);
		std::uint64_t const indexDataSize = mesh.indices.size() * sizeof(uint32_t);
		
		// make header
		AssetFileHeader const header(handle, name, MeshFileHeader{
			.vertexDataSize = vertexDataSize,
			.indexDataSize = indexDataSize
		});
		
		std::ios::sync_with_stdio(false);
		
		dest = makeAbsolute(dest);
		std::fstream file(dest, std::ios::out | std::ios::trunc | std::ios::binary);
		handleFileError(file, dest);
		
		file.write((char*)&header, sizeof(AssetFileHeader));
		file.write((char*)mesh.vertices.data(), vertexDataSize);
		file.write((char*)mesh.indices.data(), indexDataSize);
	}
	
	Reference<StaticMeshData> AssetManager::readStaticMeshFromDisk(std::filesystem::path source) {
		source = makeAbsolute(source);
		std::fstream file(source, std::ios::in | std::ios::binary);
		handleFileError(file, source);
		
		auto const header = readHeader(file);
		
		if (header.type != AssetType::staticMesh) {
			bloomLog(error, "File was not a Mesh");
			bloomDebugbreak();
			return {};
		}
	
		auto const meshHeader = header.customDataAs<MeshFileHeader>();
		
		auto const result = allocateRef<StaticMeshData>();
		result->vertices.resize(meshHeader.vertexDataSize / sizeof(Vertex3D), utl::no_init);
		result->indices.resize(meshHeader.indexDataSize  / sizeof(uint32_t), utl::no_init);
		file.read((char*)result->vertices.data(), meshHeader.vertexDataSize);
		file.read((char*)result->indices.data(),  meshHeader.indexDataSize);
		
		return result;
	}
	
	std::filesystem::path AssetManager::makeRelative(std::filesystem::path const& path) const {
		if (path.is_absolute()) {
			return std::filesystem::relative(path, workingDir());
		}
		return path;
	}
	
	std::filesystem::path AssetManager::makeAbsolute(std::filesystem::path const& path) const {
		if (path.is_relative()) {
			return workingDir() / path;
		}
		return path;
	}
	
	void AssetManager::handleFileError(std::fstream& file,
									   std::filesystem::path const& path) const
	{
		if (!file) {
			bloomDebugbreak();
			throw std::runtime_error(utl::format("Failed to open file {}", path));
		}
	}
	
}
