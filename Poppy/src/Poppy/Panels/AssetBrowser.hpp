#pragma once

#include "Panel.hpp"

#include "Bloom/Assets/Asset.hpp"

#include <filesystem>
#include <mtl/mtl.hpp>

namespace bloom { class AssetManager; }

namespace poppy {
	
	std::optional<bloom::AssetHandle> acceptAssetDragDrop(bloom::AssetType);
	
	class DirectoryView {
	public:
		struct Parameters {
			mtl::float2 itemSpacing = 10;
			mtl::float2 itemSize = 100;
			float labelHeight = 20;
		};
	public:
		void display();
		void assignDirectory(std::filesystem::path const&);
		void setAssetManager(bloom::AssetManager* m) { assetManager = m; }
		
		Parameters params;
		
	private:
		bool displayItem(std::string_view label,
						 std::optional<bloom::AssetHandle> = std::nullopt);
		void advanceCursor();
		
	private:
		utl::vector<std::string> foldersInCurrentDir;
		utl::vector<bloom::AssetHandle> assetsInCurrentDir;
		bloom::AssetManager* assetManager = nullptr;
		
	private:
		mtl::float2 contentSize = 0;
		mtl::float2 cursor = 0;
		int itemIndex = 0;
		int renaming = -1;
		bool setRenameFocus = false;
		std::array<char, 256> renameBuffer{};
	};
	
	class AssetBrowser: public Panel {
	public:
		AssetBrowser();
		
	private:
		void init() override;
		void shutdown() override;
		void display() override;
		
		void toolbar();
		
		void import(std::filesystem::path);
	
		void setWorkingDir(std::filesystem::path);
		void setCurrentDir(std::filesystem::path);
		
		void refresh();
		
	private:
		bloom::AssetManager* assetManager = nullptr;
		std::filesystem::path current; // absolute path
		DirectoryView dirView;
	};
	
}
