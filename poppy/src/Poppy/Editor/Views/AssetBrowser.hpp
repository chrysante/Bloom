#pragma once

#include <filesystem>

#include <mtl/mtl.hpp>
#include <yaml-cpp/helpers.hpp>

#include "Bloom/Asset/Asset.hpp"
#include "Poppy/Editor/Views/DirectoryView.hpp"
#include "Poppy/UI/Toolbar.hpp"
#include "Poppy/UI/View.hpp"

namespace bloom {

class AssetManager;

}

namespace poppy {

struct AssetBrowserData {
    std::filesystem::path projectDir; // absolute path
    std::filesystem::path currentDir; // absolute path
};

class AssetBrowser: public View {
public:
    AssetBrowser();

    void openAsset(bloom::AssetHandle);

private:
    void init() override;
    void shutdown() override;
    void frame() override;

    YAML::Node serialize() const override;
    void deserialize(YAML::Node) override;

    void newAssetPopup();
    void displayNoOpenProject();

    void importAsset(std::filesystem::path);

    void openProject(std::filesystem::path const&);
    void openSubdirectory(std::filesystem::path const&);
    void refreshFilesystem();

private:
    bloom::AssetManager* assetManager = nullptr;
    AssetBrowserData data;
    DirectoryView dirView;
    Toolbar toolbar;
};

} // namespace poppy

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(poppy::AssetBrowserData, projectDir, currentDir);
