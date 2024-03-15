#ifndef POPPY_EDITOR_VIEWS_ASSETBROWSER_H
#define POPPY_EDITOR_VIEWS_ASSETBROWSER_H

#include <filesystem>

#include <mtl/mtl.hpp>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Yaml.h"
#include "Poppy/Editor/Views/DirectoryView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"

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

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(poppy::AssetBrowserData, projectDir, currentDir);

#endif // POPPY_EDITOR_VIEWS_ASSETBROWSER_H
