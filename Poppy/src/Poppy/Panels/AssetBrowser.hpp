#pragma once

#include "Panel.hpp"

#include "Bloom/Assets/Asset.hpp"
#include "Poppy/Toolbar.hpp"

#include "DirectoryView.hpp"

#include <filesystem>
#include <mtl/mtl.hpp>

namespace bloom { class AssetManager; }

namespace poppy {
	
	class AssetBrowser: public Panel {
	public:
		AssetBrowser();
		
		void openAsset(bloom::AssetHandle);
		
	private:
		void init() override;
		void shutdown() override;
		void display() override;
		
//		void displayToolbar();
		
		void newAssetPopup();
		
		void import(std::filesystem::path);
	
		void setWorkingDir(std::filesystem::path wd, std::filesystem::path current = {});
		void setCurrentDir(std::filesystem::path);
		
		void refresh();
		
	private:
		bloom::AssetManager* assetManager = nullptr;
		std::filesystem::path current; // absolute path
		DirectoryView dirView;
		Toolbar toolbar;
	};
	
}
