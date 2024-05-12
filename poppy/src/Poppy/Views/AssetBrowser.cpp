#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/common.hpp>
#include <utl/filesystem_ext.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Runtime/SceneSystem.h"
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
#include "Poppy/Views/DirectoryView2.h"
#include "Poppy/Views/MaterialInstanceViewer.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

namespace {

struct AssetBrowserData {
    std::filesystem::path projectDir; // absolute path
    std::filesystem::path currentDir; // absolute path
};

static AssetHandle getHandle(DirectoryEntry2 const& entry) {
    auto* handle = std::any_cast<AssetHandle>(&entry.userData);
    return handle ? *handle : AssetHandle();
}

struct AssetBrowser: View {
    struct DirViewDelegate: DirectoryViewDelegate2 {
        AssetBrowser* This;

        explicit DirViewDelegate(AssetBrowser* This): This(This) {}

        void openFile(DirectoryEntry2 const& entry) override {
            auto handle = getHandle(entry);
            if (!handle) {
                return;
            }
            This->openAsset(handle);
        }
        std::string selectIcon(DirectoryEntry2 const& entry) override {
            auto handle = getHandle(entry);
            switch (handle.type()) {
            case AssetType::StaticMesh:
                return "cube";
            case AssetType::SkeletalMesh:
                return "cube";
            case AssetType::Material:
                return "leaf";
            case AssetType::MaterialInstance:
                return "delicious";
            case AssetType::Texture2D:
                return "file-image";
            case AssetType::Scene:
                return "cubes";
            case AssetType::ScriptSource:
                return "file-code";
            default:
                return "doc";
            }
        }
        std::any makeUserData(
            std::filesystem::directory_entry const& entry) override {
            return This->assetManager->getHandleFromFile(entry.path());
        }
        void popupMenu(DirectoryEntry2 const&) override {}

        std::string makeDragDropType() override {
            return std::string(AssetDragDropType);
        }

        std::vector<uint8_t> makeDragDropUserData(
            std::span<DirectoryEntry2 const> entries) override {
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

    void openAsset(bloom::AssetHandle);

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

    /// The applications asset manager
    bloom::AssetManager* assetManager = nullptr;
    /// The current directory relative to the project directory
    std::filesystem::path currentDir;
    AssetBrowserData data;
    DirectoryView2 dirView;
};

} // namespace

POPPY_REGISTER_VIEW(AssetBrowser, "Content Browser");

BLOOM_MAKE_TEXT_SERIALIZER(AssetBrowserData, projectDir, currentDir);

AssetBrowser::AssetBrowser() {
    // clang-format off
    setToolbar({
        ToolbarIconButton("up-open")
            .onClick([] {
                Logger::Trace("Going Up");
            })
            .tooltip("Go Up"),

        ToolbarIconButton("left-open")
            .onClick([] { Logger::Trace("Going Back"); })
            .tooltip("Go Back"),

        ToolbarIconButton("right-open")
            .onClick([] { Logger::Trace("Going Forward"); })
            .tooltip("Go Forward"),

        ToolbarSpacer{},

        ToolbarIconButton("cw")
            .onClick([this] { refreshFilesystem(); })
            .tooltip("Refresh"),

        ToolbarIconButton("angle-double-down")
            .onClick([this] {
                bloom::OpenPanelDescription desc;
                showOpenPanel(desc, [this](std::string filepath) {
                    importAsset(filepath);
                });
            })
            .tooltip("Import..."),

        ToolbarIconButton("plus")
            .onClick([] { ImGui::OpenPopup("New Asset"); })
            .tooltip("Create Asset..."),

        ToolbarIconButton("delicious")
            .onClick([this] {
                assetManager->createAsset(bloom::AssetType::Material,
                                     "Default Material",
                                     data.currentDir);
                refreshFilesystem();
            })
            .tooltip("Create Default Material"),

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

void AssetBrowser::newAssetPopup() {
    if (ImGui::BeginPopupModal("New Asset", NULL, ImGuiWindowFlags_NoTitleBar))
    {
        static AssetType type = AssetType::Invalid;
        if (ImGui::BeginCombo("Type", toString(type).data())) {
            for (unsigned i = 0; i < EnumCount<AssetType>; ++i) {
                // TODO: Check if type is concrete
                AssetType t{ i };
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
        if (ImGui::Button("Create")) {
            assetManager->createAsset(type, nameBuffer, data.currentDir);
            refreshFilesystem();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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

void AssetBrowser::openAsset(bloom::AssetHandle handle) {
    switch (handle.type()) {
    case AssetType::Material: {
        editor().openView("Material Editor", [=, this](View& view) {
            view.openData(as<Material>(assetManager->get(handle)));
            assetManager->makeAvailable(handle, AssetRepresentation::CPU);
        });
        break;
    }
    case AssetType::MaterialInstance: {
        editor().openView("Material Instance Viewer", [=, this](View& view) {
            auto* matInstViewer = dynamic_cast<MaterialInstanceViewer*>(&view);
            BL_ASSERT(matInstViewer);
            auto materialInstance =
                as<MaterialInstance>(assetManager->get(handle));
            matInstViewer->setMaterialInstance(std::move(materialInstance));
        });
        break;
    }
    case AssetType::Texture2D: {
        editor().openView("Texture Viewer", [=, this](View& view) {
            view.openData(as<Texture2D>(assetManager->get(handle)));
        });
        break;
    }
    case AssetType::Scene: {
        auto scene = as<Scene>(assetManager->get(handle));
        assetManager->makeAvailable(handle, AssetRepresentation::CPU);
        auto& sceneSystem = editor().coreSystems().sceneSystem();
        sceneSystem.unloadAll();
        sceneSystem.loadScene(std::move(scene));
        break;
    }
    case AssetType::ScriptSource: {
        auto command =
            utl::strcat("open ", assetManager->getAbsoluteFilepath(handle));
        std::system(command.data());
        break;
    }
    default:
        Logger::Trace("Cannot open asset of type: ", handle.type());
        break;
    }
}

void AssetBrowser::init() {
    assetManager = &editor().coreSystems().assetManager();
    dirView.init(std::make_unique<DirViewDelegate>(this),
                 editor().makeEmitter(), commandPaletteWrite());
    if (!data.projectDir.empty()) {
        openProject(data.projectDir);
    }
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

UI_TEST("Content Browser", "Open Scene") {
    openView("Content Browser");
    context().Yield();
    context().SetRef(
        context().WindowInfo("//Content Browser/Directory View").ID);
    context().ItemDoubleClick("###MyScene");
};
