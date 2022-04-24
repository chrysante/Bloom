#include "EditorAssetManager.hpp"

#include "Poppy/Debug.hpp"

#include "Bloom/Assets/StaticMeshAsset.hpp"

#include "MeshImporter.hpp"

namespace poppy {
	
	static bool hasExtension(std::filesystem::path p, std::string e) {
		auto a = p.extension().string();
		
		std::transform(a.begin(), a.end(), a.begin(), [](char x) { return std::toupper(x); });
		std::transform(e.begin(), e.end(), e.begin(), [](char x) { return std::toupper(x); });

		return a == e;
	}
	
	bloom::AssetHandle EditorAssetManager::import(std::filesystem::path source,
												  std::filesystem::path dest)
	{
		using namespace bloom;
		poppyAssert(source.is_absolute());
		
		poppyAssert(dest.is_relative());
//		poppyAssert(!dest.has_stem());
		
		if (workingDir().empty()) {
			poppyLog(warning, "No Working Directory specified");
			return {};
		}
		
		if (!source.has_extension()) {
			poppyLog(warning, "Failed to import file without extension: {}", source);
			return {};
		}
		
		if (hasExtension(source, ".FBX")) {
			return importStaticMesh(source, dest);
		}
		
		
		poppyLog(warning, "Unknown File Format: {}", source.extension());
		return {};
	}
	
	bloom::AssetHandle EditorAssetManager::importStaticMesh(std::filesystem::path source,
															std::filesystem::path dest)
	{
		using namespace bloom;
		namespace fs = std::filesystem;
		
		poppyAssert(source.has_filename());
		poppyAssert(source.has_extension());
		
		// import
		MeshImporter importer;
		StaticMeshData mesh = importer.import(source);
		
		std::string const name = source.filename().replace_extension();
		
		auto const targetDiskLocation = (dest / name).replace_extension(extension(AssetType::staticMesh));
		
		auto const existingAsset = findByDiskLocation(targetDiskLocation);
		if (existingAsset) {
			poppyLog("Asset at {} already exists. Replacing its content.", targetDiskLocation);
			
			flushToDisk(existingAsset, name, mesh, targetDiskLocation);
			
			reloadAsset(existingAsset);
			
			return existingAsset;
		}
		else {
			auto const handle = AssetHandle::generate(AssetType::staticMesh);
			poppyAssert(!assets.contains(handle.id()));
			auto asset = allocateRef<StaticMeshAsset>(handle, name, targetDiskLocation);
			
			flushToDisk(handle, name, mesh, targetDiskLocation);
			bool const storeResult = storeReference(asset);
			poppyAssert(storeResult);
			
			return handle;
		}
		
		
	}
	
	bloom::AssetHandle EditorAssetManager::findByDiskLocation(std::filesystem::path const& path) const {
		for (auto&& [id, asset]: assets) {
			if (asset->diskLocation().lexically_normal() == path.lexically_normal()) {
				return *asset;
			}
		}
		return {};
	}
				   
	void EditorAssetManager::reloadAsset(bloom::AssetHandle handle) {
		auto* asset = getAssetWeak(handle);
		if (!asset) {
			return;
		}
		switch (asset->type()) {
				using namespace bloom;
			case AssetType::staticMesh: {
				auto* sm = utl::down_cast<StaticMeshAsset*>(asset);
				AssetRepresentation rep{};
				if (sm->_meshData) {
					rep |= AssetRepresentation::CPU;
				}
				if (sm->_renderMesh) {
					rep |= AssetRepresentation::GPU;
				}
				makeStaticMeshAvailable(sm, rep, true);
				break;
			}
				
			default:
				poppyDebugbreak();
				break;
		}
	}
	
}


