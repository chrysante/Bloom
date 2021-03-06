#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include "AssetBrowser.hpp"
#include "MaterialInstanceViewer.hpp"

#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/ResourceUtil.hpp"

#include "Bloom/Asset/AssetManager.hpp"
#include "Bloom/Scene/Components/Script.hpp"
#include "Bloom/ScriptEngine/ScriptEngine.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"


#include "Poppy/Editor/Editor.hpp"
#include "Poppy/Core/Debug.hpp"

#include "Poppy/UI/ImGuiHelpers.hpp"
#include "Poppy/UI/Icons.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <utl/stdio.hpp>
#include <utl/filesystem_ext.hpp>
#include <utl/common.hpp>

using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
	
	static std::string toDragDropType(bloom::AssetType type) {
		return utl::format("DD-Asset-{}", type);
	}

	std::optional<bloom::AssetHandle> acceptAssetDragDrop(bloom::AssetType type) {
		using namespace bloom;
		if (ImGui::BeginDragDropTarget()) {
			utl::scope_guard endDDTarget = []{
				ImGui::EndDragDropTarget();
			};
			auto* const payload = ImGui::AcceptDragDropPayload(toDragDropType(type).data());
			
			if (payload && payload->IsPreview()) {
				poppyLog(info, "Preview");
			}
			if (payload && payload->IsDelivery()) {
				poppyLog(info, "Delivered");
				AssetHandle recievedAsset;
				std::memcpy(&recievedAsset, payload->Data, sizeof recievedAsset);
				return recievedAsset;
			}
		}
		return std::nullopt;
	}
	
	POPPY_REGISTER_VIEW(AssetBrowser, "Content Browser", {});
	
	AssetBrowser::AssetBrowser():
		dirView()
	{
		toolbar = {
			ToolbarIconButton("up-open").onClick([=]{
				dirView.goUp();
			}).enabled([=] { return dirView.canGoUp(); }).tooltip("Go Up"),
			
			ToolbarIconButton("left-open").onClick([=]{
				dirView.goBack();
			}).enabled([=] { return dirView.canGoBack(); }).tooltip("Go Back"),
			
			ToolbarIconButton("right-open").onClick([=]{
				dirView.goForward();
			}).enabled([=] { return dirView.canGoForward(); }).tooltip("Go Forward"),
			
			ToolbarSpacer{},
			
			
			ToolbarIconButton("cw").onClick([=]{
				refreshFilesystem();
			}).tooltip("Refresh"),
				
			ToolbarIconButton("angle-double-down").onClick([=]{
				OpenPanelDescription desc;
				showOpenPanel(desc, [=](std::string filepath) {
					importAsset(filepath);
				});
			}).tooltip("Import..."),

			ToolbarIconButton("plus").onClick([=]{
				ImGui::OpenPopup("New Asset");
			}).tooltip("Create Asset..."),
			
			ToolbarIconButton("delicious").onClick([=]{
				assetManager->create(AssetType::material, "Default Material", data.currentDir);
				refreshFilesystem();
			}).tooltip("Create Default Material"),
			
			ToolbarButton("Reload Scripts").onClick([=]{
				auto& scriptEngine = editor().coreSystems().scriptEngine();
				assetManager->loadScripts(scriptEngine);
				dispatch(DispatchToken::nextFrame, ScriptLoadEvent{});
			})
		};
		
		toolbar.setHeight(30);
	}
	
	void AssetBrowser::frame() {
		bool const active = assetManager && !assetManager->workingDir().empty();
//		disabledIf(!active, [&]{
//			auto& g = *GImGui;
//			auto& style = g.Style;
//			auto& window = *g.CurrentWindow;
//			auto const cp = ImGui::GetCursorPos();
//			ImGui::SetCursorPos(cp + style.WindowPadding);
//
//			toolbar.display(window.Size.x - 2 * style.WindowPadding.x);
//			ImGui::SetCursorPos(cp + ImVec2(0, toolbar.getHeight() + 2 * style.WindowPadding.y + 1));
//		});
//
//		
//		// draw separator
//		auto* window = ImGui::GetCurrentWindow();
//		auto const yPadding = GImGui->Style.WindowPadding.y;
//		auto const from = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(-yPadding, -1);
//		auto const to = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() + 2 * yPadding, -1);
//		auto* drawList = window->DrawList;
//		drawList->AddLine(from, to, ImGui::GetColorU32(ImGuiCol_Separator), 2);

		//
		if (!assetManager) {
			displayEmptyWithReason("Asset Manager not loaded");
			return;
		}
		if (assetManager->workingDir().empty()) {
			displayNoOpenProject();
			return;
		}
		if (data.projectDir.empty()) {
			data.projectDir = assetManager->workingDir();
			data.currentDir = data.projectDir;
			dirView.openDirectory(data.currentDir);
		}
		if (data.currentDir.empty()) {
			data.currentDir = data.projectDir;
			dirView.openDirectory(data.currentDir);
		}

		
		dirView.display();
		
		newAssetPopup();
	}
	
	void AssetBrowser::newAssetPopup() {
		if (ImGui::BeginPopupModal("New Asset", NULL, ImGuiWindowFlags_NoTitleBar)) {
			static AssetType type = AssetType::none;
			if (ImGui::BeginCombo("Type", toString(type).data())) {
				for (int i = 0; i < (std::size_t)AssetType::itemCount; ++i) {
					auto const t = (AssetType)(1 << i);
					bool const selected = type == t;
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
		
		bool const pressed = withFont(Font::UIDefault().setWeight(FontWeight::semibold), [&]{
			auto& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, 2 * (float2)style.FramePadding);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2 * style.FrameRounding);
			utl_defer {
				ImGui::PopStyleVar(2);
			};
			auto const buttonSize = (float2)ImGui::CalcTextSize(text) + 2 * (float2)ImGui::GetStyle().FramePadding;
			auto const cursor = ((float2)ImGui::GetContentRegionAvail() - buttonSize) / 2;
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
					poppyLog(error, "Directory does not exist: {}", dir);
				}
				openProject(dir);
			});
		}
		return;
	}
	
	void AssetBrowser::openAsset(bloom::AssetHandle handle) {
		switch (handle.type()) {
			case AssetType::materialInstance: {
				editor().openView("Material Instance Viewer", [=](View& view){
					auto* const pMatInstViewer = dynamic_cast<MaterialInstanceViewer*>(&view);
					if (!pMatInstViewer) {
						bloomDebugbreak("What?");
						return;
					}
					auto& matInstViewer = *pMatInstViewer;
					
					auto materialInstance = as<MaterialInstance>(assetManager->get(handle));
					
					matInstViewer.setMaterialInstance(std::move(materialInstance));
				});
				
				break;
			}
			case AssetType::scene: {
				auto scene = as<Scene>(assetManager->get(handle));
				assetManager->makeAvailable(handle, AssetRepresentation::CPU);
				auto& sceneSystem = editor().coreSystems().sceneSystem();
				sceneSystem.unloadAll();
				sceneSystem.loadScene(std::move(scene));
				break;
			}
			case AssetType::script: {
				auto const command = utl::format("open -a Visual\\ Studio\\ Code.app {}",
												 assetManager->getAbsoluteFilepath(handle));
				
				
				std::system(command.data());
				break;
			}
			default:
				break;
		}
	}
	
	void AssetBrowser::init() {
		setPadding({ 0, 0 });
		assetManager = &editor().coreSystems().assetManager();
		dirView.assignEmitter(editor().makeEmitter());
		
		if (!data.projectDir.empty()) {
			openProject(data.projectDir);
		}
		// Ignore subdirectory for now
	}
	
	void AssetBrowser::shutdown() {
		
	}
	
	YAML::Node AssetBrowser::serialize() const {
		YAML::Node root;
		root["Data"] = data;
		root["Directory View"] = dirView.serialize();
		return root;
	}
	
	void AssetBrowser::deserialize(YAML::Node root) {
		data = root["Data"].as<AssetBrowserData>();
		dirView.deserialize(root["Directory View"]);
	}
	
	/// MARK: -
	void AssetBrowser::importAsset(std::filesystem::path source) {
		assetManager->import(source, std::filesystem::relative(data.currentDir,
															   assetManager->workingDir()).lexically_normal());
		refreshFilesystem();
	}
	
	void AssetBrowser::openProject(std::filesystem::path const& path) {
		if (!path.is_absolute()) {
			poppyLog(error, "Can't open Project {}");
			return;
		}
		
		if (!assetManager) {
			return;
		}
		
		data.projectDir = path;
		assetManager->setWorkingDir(path);
		assetManager->loadScripts(editor().coreSystems().scriptEngine());
		dirView.setRootDirectory(path);
		openSubdirectory(path);
	}
	
	void AssetBrowser::openSubdirectory(std::filesystem::path const& path) {
		if (path.is_relative()) {
			return openSubdirectory(assetManager->workingDir() / path);
		}
		data.currentDir = path;
		dirView.openDirectory(path);
	}
	
	void AssetBrowser::refreshFilesystem() {
		assetManager->refreshWorkingDir();
		assetManager->loadScripts(editor().coreSystems().scriptEngine());
		dirView.openDirectory(data.currentDir);
	}
	
}
