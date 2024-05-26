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

struct AssetBrowser: View {
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
    bool displayPreview(AssetHandle handle);

    /// The applications asset manager
    bloom::AssetManager* assetManager = nullptr;
    /// The current directory relative to the project directory
    std::filesystem::path currentDir;
    AssetBrowserData data;
    DirectoryView dirView;
    TextureViewer textureViewer;
};

class CBDirEntry final: public DirectoryEntry {
public:
    static std::unique_ptr<DirectoryEntry> Make(
        AssetBrowser& This, AssetRegistryEntry const* regEntry,
        std::filesystem::path path) {
        return std::make_unique<CBDirEntry>(This, regEntry, std::move(path));
    }

    static std::unique_ptr<DirectoryEntry> Make(AssetBrowser& This,
                                                std::filesystem::path path) {
        auto* entry = This.assetManager->registry().findByDiskLocation(path);
        return Make(This, entry, std::move(path));
    }

    explicit CBDirEntry(AssetBrowser& This, AssetRegistryEntry const* regEntry,
                        std::filesystem::path path):
        This(This),
        regEntry(regEntry),
        _path(std::move(path)),
        _displayName(regEntry ? std::string{} : _path.filename().string()) {}

    bool isDirectory() const override { return !regEntry; }

    void erase() override {}

    void rename(std::string newName) override {
        if (regEntry) {
            BL_ASSERT(!std::filesystem::is_directory(path()));
            This.assetManager->renameAsset(regEntry->handle, newName);
        }
        else {
            BL_ASSERT(std::filesystem::is_directory(path()));
            This.assetManager->relocateFolder(path(),
                                              path().parent_path() / newName);
        }
    }

    void relocate(std::filesystem::path newLocation) override {
        if (regEntry) {
            This.assetManager->relocateAsset(regEntry->handle, newLocation);
        }
        else {
            This.assetManager->relocateFolder(path(), newLocation);
        }
    }

    std::string_view displayName() const override {
        return regEntry ? std::string_view(regEntry->name) : _displayName;
    }

    std::filesystem::path path() const override { return _path; }

    std::vector<std::unique_ptr<DirectoryEntry>> makeChildEntries() override {
        std::vector<std::unique_ptr<DirectoryEntry>> result;
        for (auto& entry: This.assetManager->registry()) {
            if (entry.diskLocation.parent_path() == _path) {
                result.push_back(Make(This, &entry, entry.diskLocation));
            }
        }
        std::error_code err;
        auto itr = std::filesystem::directory_iterator(path(), err);
        if (err) {
            Logger::Error("Filesystem error: ", err.message());
        }
        for (auto& entry: itr) {
            if (entry.is_directory()) {
                result.push_back(Make(This, nullptr, entry.path()));
            }
        }
        return result;
    }

    void createFolder(std::string name) override {
        std::filesystem::create_directory(_path / name);
    }

    void open() const override {
        BL_ASSERT(regEntry);
        openAsset(regEntry->handle);
    }

    std::string iconName() const override {
        return isDirectory() ? "folder" :
                               selectAssetIcon(regEntry->handle.type());
    }

    void contextMenu() override {}

    bool displayPreview() override {
        BL_ASSERT(!isDirectory());
        return This.displayPreview(regEntry->handle);
    }

    utl::small_vector<char> imguiDragDropUserData() const override {
        if (isDirectory()) {
            return {};
        }
        auto handle = regEntry->handle;
        utl::small_vector<char> result(sizeof handle);
        std::memcpy(result.data(), &handle, sizeof handle);
        return result;
    }

    AssetBrowser& This;
    AssetRegistryEntry const* regEntry;
    std::filesystem::path _path;
    std::string _displayName;
};

class DirViewDelegate: public DirectoryViewDelegate {
public:
    DirViewDelegate(AssetBrowser& This): This(This) {}

    std::unique_ptr<DirectoryEntry> makeEntry(
        std::filesystem::path path) override {
        return CBDirEntry::Make(This, std::move(path));
    }

    char const* imguiDragDropType() const override { return AssetDragDropType; }

    AssetBrowser& This;
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
    dirView.init(std::make_unique<DirViewDelegate>(*this),
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
        ToolbarDropdownMenu()
            .icon([] { return "sort-alt-up"; })
            .content([this] {
                using enum DirectoryView::SortCondition;
                auto cond = dirView.sortCondition();
                if (ImGui::MenuItem("Sort by Name", "", cond == Name)) {
                    dirView.setSortCondition(Name);
                }
                if (ImGui::MenuItem("Sort by Type", "", cond == Extension)) {
                    dirView.setSortCondition(Extension);
                }
            }),
        ToolbarSpacer{},
        ToolbarIconButton(dirView.getCommand(CreateFolder), "plus-circled"),
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

bool AssetBrowser::displayPreview(AssetHandle handle) {
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
