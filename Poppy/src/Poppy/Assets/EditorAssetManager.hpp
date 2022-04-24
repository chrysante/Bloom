#pragma once

#include <filesystem>

#include "Bloom/Assets/AssetManager.hpp"

namespace poppy {
	
	class EditorAssetManager: public bloom::AssetManager {
	public:
		bloom::AssetHandle import(std::filesystem::path source,
								  std::filesystem::path dest);
		
	private:
		bloom::AssetHandle importStaticMesh(std::filesystem::path source,
											std::filesystem::path dest);
		
		bloom::AssetHandle findByDiskLocation(std::filesystem::path const&) const;
		
		bool assetExists(std::filesystem::path const&) const;
		
		void reloadAsset(bloom::AssetHandle);
		
	private:
		
	};
	
}
