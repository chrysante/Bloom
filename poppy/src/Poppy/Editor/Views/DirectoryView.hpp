#pragma once

#include <optional>
#include <mtl/mtl.hpp>
#include <filesystem>

#include "Bloom/Asset/Asset.hpp"

namespace bloom { class AssetManager; }

namespace poppy {

	std::optional<bloom::AssetHandle> acceptAssetDragDrop(bloom::AssetType);

	class AssetBrowser;
	
	class DirectoryView {
	public:
		struct Parameters {
			mtl::float2 itemSpacing = 10;
			mtl::float2 itemSize = 100;
			float labelHeight = 20;
		};
	public:
		DirectoryView(AssetBrowser* browser): browser(browser) {}
		void display();
		void assignDirectory(std::filesystem::path const&);
		void setAssetManager(bloom::AssetManager* m) { assetManager = m; }
		
		Parameters params;
		
	private:
		bool displayItem(std::string_view label,
						 std::optional<bloom::AssetHandle> = std::nullopt);
		void advanceCursor(bool forceNextLine = false);
		
	private:
		utl::vector<std::string> foldersInCurrentDir;
		utl::vector<bloom::AssetHandle> assetsInCurrentDir;
		bloom::AssetManager* assetManager = nullptr;
		AssetBrowser* browser = nullptr;
		
	private:
		mtl::float2 contentSize = 0;
		mtl::float2 cursor = 0;
		int itemIndex = 0;
		int renaming = -1;
		bool setRenameFocus = false;
		std::array<char, 256> renameBuffer{};
	};

}
