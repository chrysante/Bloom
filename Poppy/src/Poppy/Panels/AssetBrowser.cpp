#include "AssetBrowser.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/Resource.hpp"
#include "Poppy/Editor.hpp"
#include "Poppy/Debug.hpp"
#include "Poppy/ResourceManager.hpp"
#include "Poppy/Assets/EditorAssetManager.hpp"

#include <utl/stdio.hpp>
#include <utl/filesystem_ext.hpp>

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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
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
		ImGui::PopStyleVar();
	}
	
	bool DirectoryView::displayItem(std::string_view label, std::optional<bloom::AssetHandle> asset) {
		mtl::float2 const labelSize = ImGui::CalcTextSize(label.data());
		auto const uniqueID = generateUniqueID(label, itemIndex, true);
		
		mtl::float2 localCursor = cursor;
		
		// Button
		localCursor += params.itemSpacing;
		ImGui::SetCursorPos(localCursor);
		
		
		ImGui::PushStyleColor(ImGuiCol_Button, 0);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFFffFF20);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFFffFF20);
		
		auto const flags = ImGuiButtonFlags_None
		| ImGuiButtonFlags_PressedOnDoubleClick
		;
		bool const result = ImGui::ButtonEx(uniqueID.data(), params.itemSize, flags);
	
		
		ImGui::PopStyleColor(3);
		
		
		if (asset) {
			if (ImGui::BeginDragDropSource()) {
				auto const handle = *asset;
				ImGui::SetDragDropPayload(toDragDropType(asset->type()).data(),
										  &handle, sizeof handle);
				ImGui::EndDragDropSource();
			}
		}
		
		// Label
		localCursor.y += params.itemSize.y - params.labelHeight / 2;
		localCursor.x += params.itemSize.x / 2;
		localCursor -= labelSize / 2;
		
		ImGui::SetCursorPos(localCursor);
		ImGui::Text("%s", label.data());
		
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
		Panel("Asset Browser")
	{
		
	}
	
	void AssetBrowser::init() {
		assetManager = utl::down_cast<EditorAssetManager*>(getApplication().getAssetManager());
		dirView.setAssetManager(assetManager);
		
		setWorkingDir(settings["Working Directory"].as<std::string>(std::string{}));
		current = settings["Current"].as<std::string>(assetManager->workingDir());
		refresh();
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
		
		toolbar();
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
	
	void AssetBrowser::toolbar() {
		float const toolbarSize = 30;
		
		{
			auto* const resourceManager = getEditor().getResourceManager();
			auto* const texture = resourceManager->getResource<TextureResource>("Icons/Icons.tga").handle.nativeHandle();
			auto const size = toolbarButtonSize(toolbarSize);
			if (ImGui::ImageButton(texture, size,
								   mtl::float2{ 0.0, 1.0 }, // uv0
								   mtl::float2{ 1./8., .5 }, // uv1
								   0))
			{
				// go up
			}
		}
		
		
		
		ImGui::SameLine();
		withFont(FontWeight::bold, FontStyle::roman, [&]{
			if (toolbarButton("Import...", toolbarSize)) {
				bloom::showOpenFilePanel([this](std::string filepath) {
					import(filepath);
				});
			}
		});
		
		ImGui::SameLine();
		{
			auto* const resourceManager = getEditor().getResourceManager();
			auto* const texture = resourceManager->getResource<TextureResource>("Icons/Icons.tga").handle.nativeHandle();
			auto const size = toolbarButtonSize(toolbarSize);
			if (ImGui::ImageButton(texture, size,
								   mtl::float2{ 1./8., 0.5 }, // uv0
								   mtl::float2{ 2./8., 1.0 }, // uv1
								   0))
			{
				refresh();
			}
		}
		
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("Preview Size", &dirView.params.itemSize.x, 30, 500);
		dirView.params.itemSize.y = dirView.params.itemSize.x;
		
		ImGui::Separator();
	}
		
	/// MARK: -
	void AssetBrowser::import(std::filesystem::path source) {
		
		assetManager->import(source, std::filesystem::relative(current, assetManager->workingDir()).lexically_normal());
	}
	
	void AssetBrowser::setWorkingDir(std::filesystem::path path) {
		if (!assetManager) {
			return;
		}
		assetManager->setWorkingDir(path);
		current = path;
	}
	
	void AssetBrowser::setCurrentDir(std::filesystem::path path) {
		current = assetManager->workingDir() / path;
		namespace fs = std::filesystem;
		
		dirView.assignDirectory(path);
	}
	
	void AssetBrowser::refresh() {
		setCurrentDir(current);
	}
	
}
