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
	
	/// MARK: - DirectoryView
	void DirectoryView::display() {
//		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::BeginChild("Asset Browser DirView");
		{
			contentSize = ImGui::GetWindowSize();
			cursor = 0;
			itemIndex = 0;
			
			for (auto&& folderName: foldersInCurrentDir) {
				if (displayItem(folderName)) {
					
				}
			}
			for (auto&& asset: assetsInCurrentDir) {
				if (displayItem(assetManager->getName(asset), asset)) {
					
				}
			}
		}
		ImGui::EndChild();
//		ImGui::PopStyleVar();
	}
	
	static auto selectIcon(bloom::AssetType t) -> std::string_view {
		using bloom::AssetType;
		
		switch (t) {
			case AssetType::staticMesh:
				return "cube";
			case AssetType::skeletalMesh:
				return "cube";
			case AssetType::material:
				return "delicious";
			case AssetType::scene:
				return "cubes";
			case AssetType::script:
				return "file-code";
				
			default:
				return "doc";
		}
	}
	
	bool DirectoryView::displayItem(std::string_view label, std::optional<bloom::AssetHandle> asset) {
		mtl::float2 const labelSize = ImGui::CalcTextSize(label.data());
		auto const uniqueID = generateUniqueID(label, itemIndex, true);
		
		auto const popupID = generateUniqueID(uniqueID.data(), itemIndex);
		
		
		mtl::float2 localCursor = cursor;
		
		// Button
		localCursor += params.itemSpacing;
		params.itemSize = 128;
		
		// button
		ImGui::PushStyleColor(ImGuiCol_Button, 0);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x20FFffFF);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x20FFffFF);
		auto const flags = ImGuiButtonFlags_PressedOnDoubleClick;
		ImGui::SetCursorPos(localCursor);
		bool const result = ImGui::ButtonEx(uniqueID.data(), params.itemSize, flags);
		ImGui::PopStyleColor(3);
		
		if (result && asset) {
			browser->openAsset(*asset);
		}
		
		// popup
		ImGui::OpenPopupOnItemClick(popupID.data());
		if (ImGui::BeginPopup(popupID.data())) {
			if (ImGui::Selectable("Rename")) {
				renaming = itemIndex;
				setRenameFocus = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		
		// dragdrop
		if (asset) {
			if (ImGui::BeginDragDropSource()) {
				auto const handle = *asset;
				ImGui::SetDragDropPayload(toDragDropType(asset->type()).data(),
										  &handle, sizeof handle);
				
				
				ImGui::PushFont((ImFont*)IconConfig::font(64));
				
				auto const iconText = IconConfig::unicodeStr(std::string(selectIcon(asset ? asset->type() : AssetType::none)));
				ImGui::Text("%s", iconText.data());
				ImGui::PopFont();
				
				ImGui::EndDragDropSource();
			}
		}
		
		// icon
		{
			ImGui::PushFont((ImFont*)IconConfig::font(64));
			auto const iconText = IconConfig::unicodeStr(std::string(selectIcon(asset ? asset->type() : AssetType::none)));
			
			auto iconCursor = localCursor;
			iconCursor.y += (params.itemSize.y - params.labelHeight) / 2;
			iconCursor.x += params.itemSize.x / 2;
			mtl::float2 const iconSize = ImGui::CalcTextSize(iconText.data());
			iconCursor -= iconSize / 2;
//			iconCursor -= framePadding / 2;
			
			ImGui::SetCursorPos(iconCursor);
			
			ImGui::Text("%s", iconText.data());
			ImGui::PopFont();
		}
		// Label
		if (itemIndex == renaming) {
			if (setRenameFocus) {
				std::strncpy(renameBuffer.data(), label.data(), renameBuffer.size() - 1);
			}
			mtl::float2 const labelSize = ImGui::CalcTextSize(renameBuffer.data());
			mtl::float2 const framePadding = mtl::float2(GImGui->Style.FramePadding);
			localCursor.y += params.itemSize.y - params.labelHeight / 2;
			localCursor.x += params.itemSize.x / 2;
			localCursor -= labelSize / 2;
			localCursor -= framePadding / 2;
			ImGui::SetCursorPos(localCursor);
			std::strncpy(renameBuffer.data(), label.data(), renameBuffer.size() - 1);
			if (setRenameFocus) {
				ImGui::SetKeyboardFocusHere();
				setRenameFocus = false;
			}
			ImGui::SetNextItemWidth(labelSize.x + 2 * framePadding.x);
			if (ImGui::InputText("##rename-field", renameBuffer.data(), renameBuffer.size(),
								 ImGuiInputTextFlags_EnterReturnsTrue))
			{
				renaming = -1;
				renameBuffer = {};
			}
			poppyLog("ItemID: {}, FocusID: {}", ImGui::GetItemID(), ImGui::GetFocusID());
		}
		else {
			localCursor.y += params.itemSize.y - params.labelHeight / 2;
			localCursor.x += params.itemSize.x / 2;
			localCursor -= labelSize / 2;
			ImGui::SetCursorPos(localCursor);
			ImGui::Text("%s", label.data());
		}
		
		
		advanceCursor();
		
		return result;
	}
	
	void DirectoryView::advanceCursor() {
		++itemIndex;
		auto const itemSizeWithSpacing = params.itemSize + 2 * params.itemSpacing;
		cursor.x += itemSizeWithSpacing.x;
		
		if (cursor.x + itemSizeWithSpacing.x > contentSize.x) {
			// we can't fit another item
			cursor.x = 0;
			cursor.y += itemSizeWithSpacing.y;
		}
	}
	
	void DirectoryView::assignDirectory(std::filesystem::path const& dir) {
		assetsInCurrentDir.clear();
		foldersInCurrentDir.clear();
		if (dir.empty()) {
			return;
		}
		
		namespace fs = std::filesystem;
		
		for (auto&& entry: fs::directory_iterator(dir)) {
			if (utl::is_hidden(entry.path())) {
				continue;
			}
			if (fs::is_regular_file(entry.path())) {
				assetsInCurrentDir.push_back(assetManager->getHandleFromFile(entry.path()));
			}
			else {
				poppyAssert(fs::is_directory(entry.path()));
				foldersInCurrentDir.push_back(entry.path().stem().string());
			}
		}
	}
	
	
	/// MARK: - AssetBrowser
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
		
		toolbar.display();
		ImGui::SetCursorPos((float2)ImGui::GetCursorPos() + float2(0, GImGui->Style.WindowPadding.y));
		ImGui::Separator();
		dirView.display();
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
//	
//	void AssetBrowser::displayToolbar() {
//		float const toolbarSize = 30;
//		
//		
//		
//		return;
//		
//		{
//			auto const size = toolbarButtonSize(toolbarSize);
//			
//			ImGui::PushFont((ImFont*)IconConfig::font(16));
//			bool const goUp = ImGui::Button(IconConfig::unicodeStr("up-big").data(), size);
//			ImGui::PopFont();
//			if (goUp)
//			{
//				// go up
//			}
//		}
//		
//		ImGui::SameLine();
//		withFont(FontWeight::bold, FontStyle::roman, [&]{
//			if (toolbarButton("Import...", toolbarSize)) {
//				bloom::showOpenFilePanel([this](std::string filepath) {
//					import(filepath);
//				});
//			}
//		});
//		
//		
//		
//		ImGui::SameLine();
//		{
//			if (toolbarButton("Refresh", toolbarSize)) {
//				refresh();
//			}
//		}
//		
//		ImGui::SameLine();
//		{
//			if (toolbarButton("New Asset...", toolbarSize)) {
//				ImGui::OpenPopup("New Asset");
//			}
//			
//				
//			if (ImGui::BeginPopupModal("New Asset", NULL, ImGuiWindowFlags_NoTitleBar)) {
//				static AssetType type = AssetType::none;
//				if (ImGui::BeginCombo("Type", toString(type).data())) {
//					for (int i = 0; i < (std::size_t)AssetType::itemCount; ++i) {
//						auto const t = (AssetType)(1 << i);
//						bool const selected = type == t;
//						if (ImGui::Selectable(toString(t).data(), selected)) {
//							type = t;
//						}
//						if (selected) {
//							ImGui::SetItemDefaultFocus();
//						}
//					}
//					
//					ImGui::EndCombo();
//				}
//				static char nameBuffer[256]{};
//				ImGui::InputText("Name", nameBuffer, std::size(nameBuffer));
//				
//				if (ImGui::Button("Cancel")) {
//					ImGui::CloseCurrentPopup();
//				}
//				ImGui::SameLine();
//				if (ImGui::Button("Create")) {
//					assetManager->create(type, nameBuffer, current);
//					refresh();
//					ImGui::CloseCurrentPopup();
//				}
//				ImGui::EndPopup();
//			}
//		}
//		ImGui::SameLine();
//		ImGui::Spacing();
//		ImGui::SameLine();
//		{
//			if (toolbarButton("Create Default Material", toolbarSize)) {
//				assetManager->create(AssetType::material, "Default Material", current);
//				refresh();
//			}
//		}
//		ImGui::SameLine();
//		{
//			if (toolbarButton("Reload Scripts", toolbarSize)) {
//				auto& scriptEngine = Application::get().scriptEngine();
//				assetManager->loadScripts(scriptEngine);
//				Application::get().publishEvent(ScriptLoadEvent{});
//			}
//		}
//		
//		ImGui::Separator();
//	}
		
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
