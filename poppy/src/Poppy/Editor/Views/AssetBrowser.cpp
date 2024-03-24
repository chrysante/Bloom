#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include <iostream>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/common.hpp>
#include <utl/filesystem_ext.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/Script.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Views/AssetBrowser.h"
#include "Poppy/Editor/Views/MaterialInstanceViewer.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

static std::string toDragDropType(AssetType type) {
    return utl::strcat("DD-Asset-", type);
}

std::optional<AssetHandle> poppy::acceptAssetDragDrop(
    std::span<AssetType const> types) {
    if (!ImGui::BeginDragDropTarget()) {
        return std::nullopt;
    }
    utl::scope_guard endTarget = [] { ImGui::EndDragDropTarget(); };
    for (auto type: types) {
        auto* payload =
            ImGui::AcceptDragDropPayload(toDragDropType(type).data());
        if (!payload) {
            continue;
        }
        if (payload->IsDelivery()) {
            AssetHandle receivedAsset;
            std::memcpy(&receivedAsset, payload->Data, sizeof receivedAsset);
            return receivedAsset;
        }
    }
    return std::nullopt;
}

std::optional<AssetHandle> poppy::acceptAssetDragDrop(
    std::initializer_list<AssetType> types) {
    return acceptAssetDragDrop(std::span(types));
}

POPPY_REGISTER_VIEW(AssetBrowser, "Content Browser", {});

static AssetHandle getHandle(DirectoryEntry const& entry) {
    auto* handle = std::any_cast<AssetHandle>(&entry.userData);
    return handle ? *handle : AssetHandle();
}

AssetBrowser::AssetBrowser() {
    // clang-format off
    dirView.setDelegate({
        .openFile = [this](DirectoryEntry const& entry) {
            auto handle = getHandle(entry);
            if (!handle) {
                return;
            }
            openAsset(handle);
        },
        .renameEntry = [this](DirectoryEntry const& entry, 
                              std::string newName) {
            auto handle = getHandle(entry);
            /// Kinda hacky but the asset manager expects a name without
            /// extension for renaming, so we just discard the extension here
            auto name = std::filesystem::path(newName).replace_extension()
                            .string();
            assetManager->renameAsset(handle, name);
        },
        .selectIcon = [](DirectoryEntry const& entry) {
            auto handle = getHandle(entry);
            switch (handle.type()) {
            case AssetType::StaticMesh:
                return "cube";
            case AssetType::SkeletalMesh:
                return "cube";
            case AssetType::Material:
                return "delicious";
            case AssetType::MaterialInstance:
                return "delicious";
            case AssetType::Scene:
                return "cubes";
            case AssetType::ScriptSource:
                return "file-code";
            default:
                return "doc";
            }
        },
        .shallDisplay = [](DirectoryEntry const& entry) {
            return getHandle(entry).isValid();
        },
        .makeUserData = [this](std::filesystem::path const& path) {
            return assetManager->getHandleFromFile(path);
        },
        .popupMenu = [](DirectoryEntry const&) {},
        .dragdropPayload = [](DirectoryEntry const& entry) {
            if (auto handle = getHandle(entry)) {
                ImGui::SetDragDropPayload(toDragDropType(handle.type()).data(),
                                          &handle, sizeof handle);
            }
        },
    }); // clang-format on
    // clang-format off
    toolbar = {
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
    };  // clang-format on
    toolbar.setHeight(30);
    setPadding({ -1, 0 });
}

void AssetBrowser::frame() {
    bool active = assetManager && !assetManager->projectRootDir().empty();
    poppy::disabledIf(!active, [&] {
        /// Add x window padding in y direction because y padding has been
        /// pushed to  0
        ImGui::SetCursorPos(ImGui::GetCursorPos() +
                            ImVec2(0, GImGui->Style.WindowPadding.y));
        toolbar.display();
        /// Add x window padding in y direction because y padding has been
        /// pushed to 0
        ImGui::SetCursorPos(ImGui::GetCursorPos() +
                            ImVec2(0, GImGui->Style.WindowPadding.y + 1));
    });
    if (!assetManager) {
        displayEmptyWithReason("Asset Manager not loaded");
        return;
    }
    if (assetManager->projectRootDir().empty()) {
        displayNoOpenProject();
        return;
    }
    newAssetPopup();
    // Draw separator
    auto* window = ImGui::GetCurrentWindow();
    auto yPadding = GImGui->Style.WindowPadding.y;
    auto from =
        ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(-yPadding, -1);
    auto to = ImGui::GetWindowPos() + ImGui::GetCursorPos() +
              ImVec2(ImGui::GetWindowWidth() + 2 * yPadding, -1);
    auto* drawList = window->DrawList;
    drawList->AddLine(from, to, ImGui::GetColorU32(ImGuiCol_Separator), 2);
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
    char const* const text = "Choose Working Directory...";

    bool const pressed =
        withFont(FontDesc::UIDefault().setWeight(FontWeight::Semibold), [&] {
        auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            2 * (float2)style.FramePadding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                            2 * style.FrameRounding);
        utl_defer { ImGui::PopStyleVar(2); };
        auto const buttonSize = (float2)ImGui::CalcTextSize(text) +
                                2 * (float2)ImGui::GetStyle().FramePadding;
        auto const cursor =
            ((float2)ImGui::GetContentRegionAvail() - buttonSize) / 2;
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
    if (!data.projectDir.empty()) {
        openProject(data.projectDir);
    }
}

void AssetBrowser::shutdown() {}

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
