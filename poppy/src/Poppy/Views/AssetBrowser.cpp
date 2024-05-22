#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/common.hpp>
#include <utl/filesystem_ext.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Scene/Components/Script.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/TestRegistry.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/DirectoryView.h"
#include "Poppy/Views/TextureViewer.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

namespace {

struct AssetBrowserData {
    std::filesystem::path projectDir; // absolute path
    std::filesystem::path currentDir; // absolute path
};

static AssetHandle getHandle(DirectoryEntry const& entry) {
    auto* handle = std::any_cast<AssetHandle>(&entry.userData);
    return handle ? *handle : AssetHandle();
}

struct AssetBrowser: View {
    struct DirViewDelegate: DirectoryViewDelegate {
        AssetBrowser* This;

        explicit DirViewDelegate(AssetBrowser* This): This(This) {}

        void openFile(DirectoryEntry const& entry) override {
            auto handle = getHandle(entry);
            if (!handle) {
                return;
            }
            openAsset(handle);
        }
        void renameEntry(DirectoryEntry const& entry,
                         std::string newName) override {
            auto handle = getHandle(entry);
            /// Kinda hacky but the asset manager expects a name without
            /// extension for renaming, so we just discard the extension here
            auto name =
                std::filesystem::path(newName).replace_extension().string();
            This->assetManager->renameAsset(handle, name);
        }
        std::string selectIcon(DirectoryEntry const& entry) override {
            auto handle = getHandle(entry);
            return selectAssetIcon(handle.type());
        }
        std::any makeUserData(
            std::filesystem::directory_entry const& entry) override {
            return This->assetManager->getHandleFromFile(entry.path());
        }
        void entryContextMenu(DirectoryEntry const&) override {}
        bool displayPreview(DirectoryEntry const& entry) override {
            return This->displayPreview(entry);
        }
        std::string makeDragDropType() override {
            return std::string(AssetDragDropType);
        }
        std::vector<uint8_t> makeDragDropUserData(
            std::span<DirectoryEntry const> entries) override {
            if (entries.size() == 1) {
                auto handle = getHandle(entries.front());
                std::vector<uint8_t> result(sizeof handle);
                std::memcpy(result.data(), &handle, sizeof handle);
                return result;
            }
            return {};
        }
    };

    AssetBrowser();

    void importFiles();

    void init() override;
    void displayToolbar() override;
    void display() override;

    YAML::Node serialize() const override;
    void deserialize(YAML::Node) override;

    void newAssetPopup();
    void displayNoOpenProject();

    void importAsset(std::filesystem::path);

    void openProject(std::filesystem::path const&);
    void openSubdirectory(std::filesystem::path const&);
    void refreshFilesystem();
    bool displayPreview(DirectoryEntry const& entry);

    /// The applications asset manager
    bloom::AssetManager* assetManager = nullptr;
    /// The current directory relative to the project directory
    std::filesystem::path currentDir;
    AssetBrowserData data;
    DirectoryView dirView;
    TextureViewer textureViewer;
};

} // namespace

POPPY_REGISTER_VIEW(AssetBrowser, "Content Browser");

BLOOM_MAKE_TEXT_SERIALIZER(AssetBrowserData, projectDir, currentDir);

AssetBrowser::AssetBrowser() = default;

void AssetBrowser::importFiles() {
    bloom::OpenPanelDescription desc = { .allowsMultipleSelection = true };
    showOpenPanel(desc,
                  [this](std::span<std::filesystem::path const> filepaths) {
        for (auto& path: filepaths) {
            importAsset(path);
        }
    });
}

void AssetBrowser::init() {
    assetManager = &editor().coreSystems().assetManager();
    dirView.init(std::make_unique<DirViewDelegate>(this),
                 editor().makeEmitter(), commandPaletteWrite());
    if (!data.projectDir.empty()) {
        openProject(data.projectDir);
    }
    using enum DirectoryView::CommandType;
    // clang-format off
    setToolbar({
        ToolbarIconButton(dirView.getCommand(GoBack), "left-open"),
        ToolbarIconButton(dirView.getCommand(GoForward), "right-open"),
        ToolbarIconButton(dirView.getCommand(GoUp), "up-open"),
        ToolbarSpacer{},
        ToolbarIconButton(dirView.getCommand(Rescan), "cw"),
        ToolbarIconButton("download")
            .onClick([this] { importFiles(); })
            .tooltip("Import..."),
        ToolbarIconButton("plus")
            .onClick([] { ImGui::OpenPopup("New Asset"); })
            .tooltip("Create Asset..."),
        ToolbarButton("Compile Scripts")
            .onClick([this] {
                assetManager->compileScripts();
            })
    }); // clang-format on
    setPadding({ 0, 0 });
}

void AssetBrowser::displayToolbar() {
    bool active = assetManager && !assetManager->projectRootDir().empty();
    ImGui::BeginDisabled(!active);
    View::displayToolbar();
    ImGui::EndDisabled();
}

void AssetBrowser::display() {
    if (!assetManager) {
        displayEmptyWithReason("Asset Manager Not Loaded");
        return;
    }
    if (assetManager->projectRootDir().empty()) {
        displayNoOpenProject();
        return;
    }
    newAssetPopup();
    dirView.display();
}

static bool isConcreteAsset(AssetType t) {
    switch (t) {
#define BLOOM_ASSET_TYPE_DEF(Name, ...)                                        \
    case AssetType::Name:                                                      \
        return utl::dc::IDIsConcrete<AssetType::Name>;
#include "Bloom/Asset/Assets.def"
    }
}

void AssetBrowser::newAssetPopup() {
    if (!ImGui::BeginPopupModal("New Asset", NULL, ImGuiWindowFlags_NoTitleBar))
    {
        return;
    }
    static AssetType type = AssetType::Invalid;
    if (ImGui::BeginCombo("Type", toString(type).data())) {
        for (unsigned i = 0; i < EnumCount<AssetType>; ++i) {
            AssetType t{ i };
            if (!isConcreteAsset(t)) {
                continue;
            }
            bool selected = type == t;
            if (ImGui::Selectable(toString(t).data(), selected)) {
                type = t;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    static char nameBuffer[256]{};
    ImGui::InputText("Name", nameBuffer, std::size(nameBuffer));
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!isConcreteAsset(type));
    if (ImGui::Button("Create")) {
        assetManager->createAsset(type, nameBuffer, data.currentDir);
        refreshFilesystem();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::EndPopup();
}

void AssetBrowser::displayNoOpenProject() {
    char const* text = "Choose Working Directory...";
    bool pressed =
        withFont(FontDesc::UIDefault().setWeight(FontWeight::Semibold), [&] {
        auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            2 * (float2)style.FramePadding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                            2 * style.FrameRounding);
        utl_defer { ImGui::PopStyleVar(2); };
        auto buttonSize = (float2)ImGui::CalcTextSize(text) +
                          2 * (float2)ImGui::GetStyle().FramePadding;
        auto cursor = ((float2)ImGui::GetContentRegionAvail() - buttonSize) / 2;
        ImGui::SetCursorPos(cursor);
        return ImGui::Button(text);
    });
    if (pressed) {
        OpenPanelDescription desc;
        desc.canChooseFiles = false;
        desc.canChooseDirectories = true;
        desc.allowsMultipleSelection = false;
        showOpenPanel(desc, [this](std::filesystem::path const& dir) {
            if (!std::filesystem::exists(dir)) {
                Logger::Error("Directory does not exist: ", dir);
            }
            openProject(dir);
        });
    }
    return;
}

YAML::Node AssetBrowser::serialize() const {
    YAML::Node node;
    node = data;
    return node;
}

void AssetBrowser::deserialize(YAML::Node node) {
    data = node.as<AssetBrowserData>();
}

/// MARK: -
void AssetBrowser::importAsset(std::filesystem::path source) {
    auto dest = std::filesystem::relative(data.currentDir,
                                          assetManager->projectRootDir())
                    .lexically_normal();
    auto name = source.filename().replace_extension().string();
    assetManager->importAsset(name, source, dest);
    refreshFilesystem();
}

void AssetBrowser::openProject(std::filesystem::path const& path) {
    if (!path.is_absolute()) {
        Logger::Error("Can't open Project: ", path);
        return;
    }
    if (!assetManager) {
        return;
    }
    data.projectDir = path;
    assetManager->openProject(path);
    openSubdirectory(path);
}

void AssetBrowser::openSubdirectory(std::filesystem::path const& path) {
    if (path.is_relative()) {
        return openSubdirectory(assetManager->projectRootDir() / path);
    }
    data.currentDir = path;
    dirView.openDirectory(path);
}

void AssetBrowser::refreshFilesystem() {
    assetManager->refreshProject();
    dirView.openDirectory(data.currentDir);
}

bool AssetBrowser::displayPreview(DirectoryEntry const& entry) {
    AssetHandle handle = getHandle(entry);
    if (!handle) {
        return false;
    }
    if (handle.type() == AssetType::Texture2D) {
        if (!textureViewer.getTexture() ||
            textureViewer.getTexture()->handle() != handle)
        {
            auto asset = assetManager->get(handle, AssetRepresentation::GPU);
            textureViewer =
                TextureViewer(as<Texture2D>(asset),
                              { .displayMode =
                                    TextureViewer::DisplayMode::Compact });
        }
        textureViewer.display();
        return true;
    }
    return false;
}

UI_TEST("Content Browser", "Open Scene") {
    openView("Content Browser");
    context().Yield();
    context().SetRef(
        context().WindowInfo("//Content Browser/Directory View").ID);
    context().ItemDoubleClick("###MyScene");
};
