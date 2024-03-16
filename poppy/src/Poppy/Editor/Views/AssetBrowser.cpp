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
#include "Poppy/UI/Icons.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

POPPY_REGISTER_VIEW(AssetBrowser, "Content Browser", {});

AssetBrowser::AssetBrowser(): dirView(this) {
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
            .onClick([this] { ImGui::OpenPopup("New Asset"); })
            .tooltip("Create Asset..."),

        ToolbarIconButton("delicious")
            .onClick([this] {
                assetManager->create(bloom::AssetType::Material,
                                     "Default Material",
                                     data.currentDir);
                refreshFilesystem();
            })
            .tooltip("Create Default Material"),

        ToolbarButton("Compile Scripts")
            .onClick([this] {
                assetManager->compileScripts();
            })
    }; // clang-format on
    toolbar.setHeight(30);
    setPadding({ -1, 0 });
}

void AssetBrowser::frame() {
    bool const active = assetManager && !assetManager->workingDir().empty();
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
    if (assetManager->workingDir().empty()) {
        displayNoOpenProject();
        return;
    }

    newAssetPopup();

    // draw separator
    auto* window = ImGui::GetCurrentWindow();
    auto const yPadding = GImGui->Style.WindowPadding.y;
    auto const from =
        ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(-yPadding, -1);
    auto const to = ImGui::GetWindowPos() + ImGui::GetCursorPos() +
                    ImVec2(ImGui::GetWindowWidth() + 2 * yPadding, -1);
    auto* drawList = window->DrawList;
    drawList->AddLine(from, to, ImGui::GetColorU32(ImGuiCol_Separator), 2);

    dirView.display();
}

void AssetBrowser::newAssetPopup() {
    if (ImGui::BeginPopupModal("New Asset", NULL, ImGuiWindowFlags_NoTitleBar))
    {
        static AssetType type = InvalidAssetType;
        if (ImGui::BeginCombo("Type", toString(type).data())) {
            for (unsigned i = 0; i < EnumCount<AssetType>; ++i) {
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
            assetManager->create(type, nameBuffer, data.currentDir);
            refreshFilesystem();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void AssetBrowser::displayNoOpenProject() {
    char const* const text = "Choose Working Directory...";

    bool const pressed =
        withFont(Font::UIDefault().setWeight(FontWeight::semibold), [&] {
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
        break;
    }
}

void AssetBrowser::init() {
    assetManager = &editor().coreSystems().assetManager();
    dirView.setAssetManager(assetManager);
    if (!data.projectDir.empty()) {
        openProject(data.projectDir);
    }
    // Ignore subdirectories for now
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
    assetManager->import(source,
                         std::filesystem::relative(data.currentDir,
                                                   assetManager->workingDir())
                             .lexically_normal());
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
    assetManager->setWorkingDir(path);
    openSubdirectory(path);
}

void AssetBrowser::openSubdirectory(std::filesystem::path const& path) {
    if (path.is_relative()) {
        return openSubdirectory(assetManager->workingDir() / path);
    }
    data.currentDir = path;
    dirView.assignDirectory(path);
}

void AssetBrowser::refreshFilesystem() {
    assetManager->refreshWorkingDir();
    dirView.assignDirectory(data.currentDir);
}
