#define IMGUI_DEFINE_MATH_OPERATORS

#include "AssetBrowser.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/Resource.hpp"
#include "Bloom/Assets/AssetManager.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"
#include "Bloom/Scene/Components/Script.hpp"
#include "Bloom/ScriptEngine/ScriptEngine.hpp"

#include "Poppy/Editor/Editor.hpp"
#include "Poppy/Debug.hpp"
#include "Poppy/ResourceManager.hpp"

#include <utl/stdio.hpp>
#include <utl/filesystem_ext.hpp>

#include "Poppy/IconConfig.hpp"

using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
	
	AssetBrowser::AssetBrowser():
		Panel("Asset Browser", PanelOptions{ .unique = false }),
		dirView(this)
	{
		toolbar = {
			ToolbarIconButton{ [this]{ }, []{ return "up-open"; }, []{ return "Go up"; } },
			ToolbarIconButton{ [this]{ }, []{ return "left-open"; }, []{ return "Go back"; } },
			ToolbarIconButton{ [this]{ }, []{ return "right-open"; }, []{ return "Go forward"; } },
			
			ToolbarSpacer{},
			ToolbarButton{ "Refresh", [this]{ refresh(); } },
			
			ToolbarIconButton{
				[this]{
					bloom::showOpenFilePanel([this](std::string filepath) {
						import(filepath);
					});
				},
				[]{ return "down-open"; },
				[]{ return "Import..."; },
			},
			ToolbarIconButton{
				[this]{ ImGui::OpenPopup("New Asset"); },
				[]{ return "plus"; },
				[]{ return "Create Asset..."; }
			},
			ToolbarIconButton{
				[this]{
					assetManager->create(AssetType::material, "Default Material", current);
					refresh();
				},
				[]{ return "delicious"; },
				[]{ return "Create Default Material"; }
			},
			ToolbarButton{ "Reload Scripts", [this]{
				auto& scriptEngine = Application::get().scriptEngine();
				assetManager->loadScripts(scriptEngine);
				Application::get().publishEvent(ScriptLoadEvent{});
			} }
		};
		
		toolbar.setHeight(30);
		
		padding.y = 0;
	}
	
	void AssetBrowser::display() {
		if (!assetManager) {
			return;
		}
		if (assetManager->workingDir().empty()) {
			if (ImGui::Button("Choose Working Directory...")) {
				bloom::showSelectDirectoryPanel([this](std::string filepath) {
					assert(std::filesystem::exists(filepath));
					setWorkingDir(filepath);
					settings["Asset Directory"] = filepath;
				});
			}
			return;
		}
		
		newAssetPopup();
		
		// add x window padding in y direction because y padding has been pushed to 0
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, GImGui->Style.WindowPadding.x));

		toolbar.display();

		// add x window padding in y direction because y padding has been pushed to 0
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, GImGui->Style.WindowPadding.x + 1));
		
		// draw separator
		auto* window = ImGui::GetCurrentWindow();
		auto const yPadding = GImGui->Style.WindowPadding.y;
		auto const from = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(-yPadding, -1);
		auto const to = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() + 2 * yPadding, -1);
		auto* drawList = window->DrawList;
		drawList->AddLine(from, to, ImGui::GetColorU32(ImGuiCol_Separator), 2);
		
		dirView.display();
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
				assetManager->create(type, nameBuffer, current);
				refresh();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
	
	void AssetBrowser::openAsset(bloom::AssetHandle handle) {
		switch (handle.type()) {
			case AssetType::scene: {
				auto scene = as<SceneAsset>(assetManager->get(handle));
				assetManager->makeAvailable(handle, AssetRepresentation::CPU);
				Editor::get().setScene(scene);
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
		assetManager = &Application::get().assetManager();
		dirView.setAssetManager(assetManager);
		
		setWorkingDir(settings["Working Directory"].as<std::string>(std::string{}),
					  settings["Current"].as<std::string>(std::filesystem::path{}));
	}
	
	void AssetBrowser::shutdown() {
		settings["Working Directory"] = assetManager->workingDir().string();
		settings["Current"] = current.string();
	}
	
	static mtl::float2 toolbarButtonSize(float height, char const* label = nullptr) {
		mtl::float2 size = ImGui::CalcTextSize(label);
		size += 2 * mtl::float2(GImGui->Style.FramePadding);
		size[0] = std::max(size[0], height);
		size[1] = std::max(size[1], height);
		return size;
	}
	
	static bool toolbarButton(char const* label, float height) {
		return ImGui::Button(label, toolbarButtonSize(height, label));
	}
	
	/// MARK: -
	void AssetBrowser::import(std::filesystem::path source) {
		assetManager->import(source, std::filesystem::relative(current, assetManager->workingDir()).lexically_normal());
		refresh();
	}
	
	void AssetBrowser::setWorkingDir(std::filesystem::path wd, std::filesystem::path current) {
		if (!assetManager) {
			return;
		}
		assetManager->setWorkingDir(wd);
		assetManager->loadScripts(Application::get().scriptEngine());
		if (current.empty()) {
			current = wd;
		}
		setCurrentDir(current);
	}
	
	void AssetBrowser::setCurrentDir(std::filesystem::path path) {
		if (path.is_relative()) {
			current = assetManager->workingDir() / path;
		}
		else {
			current = path;
		}
		
		namespace fs = std::filesystem;
		
		dirView.assignDirectory(path);
	}
	
	void AssetBrowser::refresh() {
		assetManager->refreshWorkingDir();
		assetManager->loadScripts(Application::get().scriptEngine());
		dirView.assignDirectory(assetManager->workingDir() / current);
	}
	
}
