#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include "DirectoryView.hpp"

#include "Poppy/Core/Debug.hpp"
#include "Poppy/UI/ImGuiHelpers.hpp"
#include "Poppy/UI/ImGuiContext.hpp"

#include "Bloom/Application/Commands.hpp"

#include <utl/format.hpp>
#include <utl/scope_guard.hpp>
#include <utl/filesystem_ext.hpp>
#include <utl/hashset.hpp>
#include <numeric>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>


using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
    
	/// MARK: Default Delegate
	DirectoryItemDescription DirectoryViewDelegate::makeItemDescription(std::filesystem::directory_entry const& entry) const {
		DirectoryItemDescription desc;
		auto& path = entry.path();
		desc.label = path.has_filename() ? path.filename().string() : path.has_stem() ? path.stem().string() : "?";
		
		desc.iconName = std::filesystem::is_directory(path) ? "folder-open-empty" : "doc";
		return desc;
	}
	
	
	/// MARK: - DirectoryView
	///
	///
	DirectoryView::DirectoryView() {
		toolbar = {
			ToolbarIconButton("up-open").onClick([=]{
				goUp();
			}).enabled([=] { return canGoUp(); })
				.tooltip("Go Up")
				.additionalButtonFlags(ImGuiButtonFlags_PressedOnDragDropHold),
			
			ToolbarIconButton("left-open").onClick([=]{
				goBack();
			}).enabled([=] { return canGoBack(); })
				.tooltip("Go Back")
				.additionalButtonFlags(ImGuiButtonFlags_PressedOnDragDropHold),
			
			ToolbarIconButton("right-open").onClick([=]{
				goForward();
			}).enabled([=] { return canGoForward(); })
				.tooltip("Go Forward")
				.additionalButtonFlags(ImGuiButtonFlags_PressedOnDragDropHold),
			
			CustomToolbarElement([=](mtl::float2 size) {
				addressbar(size);
			}),
			
			ToolbarIconButton("cw").onClick([=]{
				rescan();
			}).tooltip("Rescan")
		};
		
		toolbar.setHeight(30);
	}
	
	void DirectoryView::display(DirectoryViewDisplayDescription const& displayDesc) {
		if (!delegate) {
			delegate = std::make_unique<DirectoryViewDelegate>();
		}
		
		if (!displayDesc.noToolbar) {
			auto& g = *GImGui;
			auto& style = g.Style;
			auto& window = *g.CurrentWindow;
			auto const cp = ImGui::GetCursorPos();
			ImGui::SetCursorPos(cp + style.WindowPadding);

			toolbar.display(window.Size.x - 2 * style.WindowPadding.x);
			ImGui::SetCursorPos(cp + ImVec2(0, toolbar.getHeight() + 2 * style.WindowPadding.y + 1));
			
			// draw separator
			auto const yPadding = GImGui->Style.WindowPadding.y;
			auto const from = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(-yPadding, -1);
			auto const to = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() + 2 * yPadding, -1);
			auto* drawList = window.DrawList;
			drawList->AddLine(from, to, ImGui::GetColorU32(ImGuiCol_Separator), 2);
		}
		
		ImGui::BeginChild("directory-view");
	
		displayTableView();
		
		displayBackground();
		
		ImGui::EndChild();
	
		history.displayDebugger();
	}
	
	/// MARK: Queries
	///
	///
	std::filesystem::path DirectoryView::currentDirectory() const {
		return currDir().path;
	}
	
	std::filesystem::path DirectoryView::rootDirectory() const {
		return rootDirPath;
	}
	
	bool DirectoryView::canGoUp() const {
		return !directoryLine.empty() && currentDirectory() != rootDirPath;
	}
	
	bool DirectoryView::canGoBack() const {
		return history.canPop();
	}
	
	bool DirectoryView::canGoForward() const {
		return history.canUnpop();
	}
	
	/// MARK: Modifiers
	///
	///
	YAML::Node DirectoryView::serialize() const {
		YAML::Node root;
		root = desc;
		return root;
	}
	
	void DirectoryView::deserialize(YAML::Node const& root) {
		desc = root.as<Description>();
	}
	
	void DirectoryView::rescan() {
		try {
			scanDirectoryLine();
		}
		catch (std::filesystem::filesystem_error const&) {
			auto current = currentDirectory();
			while (!std::filesystem::exists(current)) {
				current = current.parent_path();
			}
			history.clear();
			bool const result = setDirectory(current); (void)result;
			poppyAssert(result);
		}
	}
	
	void DirectoryView::openDirectory(std::filesystem::path target) {
		if (!setDirectory(target)) {
			throw std::runtime_error("Failed to open Directory");
		}
		history.clear();
		history.push(target);
	}
	
	void DirectoryView::setRootDirectory(std::filesystem::path path) {
		rootDirPath = path;
	}
	
	void DirectoryView::goUp() {
		poppyAssert(canGoUp());
		auto const target = currentDirectory().parent_path();
		
		if (!setDirectory(target)) {
			throw std::runtime_error("Failed to open Directory");
		}
		history.push(target);
	}
	
	void DirectoryView::goBack() {
		history.pop();
		setDirectory(history.current());
	}
	
	void DirectoryView::goForward() {
		history.unpop();
		setDirectory(history.current());
	}
	
	void DirectoryView::setSortCondition(SortCondition condition) {
		desc.sortCondition = condition;
		scanDirectoryLine();
	}
	
	/// MARK: Private
	///
	///
	void DirectoryView::addressbar(mtl::float2 size) {
		auto const currentDir = std::filesystem::relative(currentDirectory(), rootDirPath);
		
		auto& g = *GImGui;
		auto& style = g.Style;
		auto& window = *ImGui::GetCurrentWindow();

		float const buttonHeight = g.FontSize + 2 * style.FramePadding.y;
		
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y - buttonHeight) / 2);
		
		auto const rootPos = window.DC.CursorPos;
		float totalWidth = 0;
		
		ImGui::PushID("toolbar-address-bar");
		
		std::size_t const dirCount = directoryLine.size();
		
		utl::small_vector<float> buttonWidths(dirCount);
		for (auto [index, width]: utl::enumerate(buttonWidths)) {
			auto const labelWidth = ImGui::CalcTextSize(directoryLine[index].path.filename().c_str()).x;
			width = labelWidth + style.FramePadding.x * 2;
			if (index > 0) {
				width += style.FramePadding.x * 2;
			}
			totalWidth += width;
		}
		
		for(auto&& [index, directory]: utl::enumerate(directoryLine)) {
			ImGui::PushID(index);
			utl_defer{ ImGui::PopID(); };
			if (index > 0) {
				ImGui::SameLine(0, 0);
			}
			auto const& currentPath = directory.path;
			auto const label = currentPath.filename().string();
			ImVec2 const pos = window.DC.CursorPos;
			ImVec2 const buttonSize = { buttonWidths[index], buttonHeight };
			
			ImRect const bb(pos, pos + buttonSize);
			ImGuiID const id = window.GetID(index);
			ImGui::ItemSize(buttonSize);
			if (!ImGui::ItemAdd(bb, id)) {
				continue;
			}
			bool hovered, held;
			bool const activated = ImGui::ButtonBehavior(bb, id, &hovered, &held,
														 ImGuiButtonFlags_PressedOnClick |
														 ImGuiButtonFlags_PressedOnRelease |
														 ImGuiButtonFlags_MouseButtonLeft |
														 ImGuiButtonFlags_MouseButtonRight |
														 ImGuiButtonFlags_PressedOnDragDropHold);
			bool const leftMousePressed = activated && g.IO.MouseReleased[ImGuiMouseButton_Left] && !g.DragDropActive;
			bool const rightMousePressed = activated && g.IO.MouseClicked[ImGuiMouseButton_Right] && !g.DragDropActive;
			bool const dragdropPressed = activated &&
				!g.IO.MouseClicked[ImGuiMouseButton_Left] &&
				!g.IO.MouseReleased[ImGuiMouseButton_Left] &&
				!g.IO.MouseClicked[ImGuiMouseButton_Right] &&
				!g.IO.MouseReleased[ImGuiMouseButton_Right];
		
			
			
			if ((leftMousePressed || dragdropPressed) && index != directoryLine.size() - 1) {
				gotoNextFrame(currentPath);
			}
			
			itemDragTarget(Entry{ std::filesystem::directory_entry(directory.path) });
			
			auto const popupPosition = pos + ImVec2(index == 0 ? 0 : style.FramePadding.x, buttonSize.y);
			addressbarPopup(rightMousePressed, index, popupPosition);
		
			// Draw Button
			auto const color = ImGui::GetColorU32(held ?
												  ImGuiCol_FrameBgActive :
												  hovered ?
												  ImGuiCol_FrameBgHovered :
												  ImGuiCol_FrameBg);
			drawAddressbarButton(index, pos, buttonSize, color, label);
		}
		// draw outer border here since its hard to extract into drawAddressbarButton()
		if (style.FrameBorderSize) {
			window.DrawList->AddRect(rootPos, rootPos + ImVec2(totalWidth, buttonHeight),
									 ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);
		}
		
		ImGui::PopID();
	}
	
	void DirectoryView::drawAddressbarButton(std::size_t const index,
											 mtl::float2 const position,
											 mtl::float2 const buttonSize,
											 std::uint32_t const color,
											 std::string_view label) const
	{
		auto& style = GImGui->Style;
		auto& window = *GImGui->CurrentWindow;
		float2 const framePadding = style.FramePadding;
		float const rounding = style.FrameRounding;
		bool const haveBorder = style.FrameBorderSize;
		
		auto const borderColor = ImGui::GetColorU32(ImGuiCol_Border);
		auto& drawlist = *window.DrawList;
		
		auto const drawInnerBorder = [&]{
			drawlist.PathLineTo(position + float2(buttonSize.x - framePadding.x + 1, 1));
			drawlist.PathLineTo(position + float2(buttonSize.x + framePadding.x, buttonSize.y / 2));
			drawlist.PathLineTo(position + float2(buttonSize.x - framePadding.x + 1, buttonSize.y - 1));
			drawlist.PathStroke(borderColor);
		};
		
		if (directoryLine.size() == 1) { // Single Button
			drawlist.AddRectFilled(position, position + buttonSize, color, rounding);
			drawlist.AddText(position + (float2)style.FramePadding,
							 ImGui::GetColorU32(ImGuiCol_Text),
							 label.data(), label.data() + label.size());
		}
		else if (index == 0) { // Leftmost Button
			drawlist.AddRectFilled(position, position + buttonSize - float2(2 * framePadding.x, 0), color, rounding, ImDrawFlags_RoundCornersLeft);
			drawlist.PathLineTo(position + float2(buttonSize.x - 2 * framePadding.x, 0));
			drawlist.PathLineTo(position + float2(buttonSize.x - framePadding.x, 0));
			drawlist.PathLineTo(position + float2(buttonSize.x + framePadding.x, buttonSize.y / 2));
			drawlist.PathLineTo(position + float2(buttonSize.x - framePadding.x, buttonSize.y));
			drawlist.PathLineTo(position + float2(buttonSize.x - 2 * framePadding.x, buttonSize.y));
			drawlist.PathFillConvex(color);
			
			if (haveBorder) {
				drawInnerBorder();
			}
			
			drawlist.AddText(position + framePadding,
							 ImGui::GetColorU32(ImGuiCol_Text),
							 label.data(), label.data() + label.size());
		}
		else if (index < directoryLine.size() - 1) { // Central Buttons
			drawlist.PathLineTo(position + float2(-framePadding.x, 0));
			drawlist.PathLineTo(position + float2(framePadding.x, 0));
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y / 2));
			drawlist.PathFillConvex(color);
			
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y / 2));
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y));
			drawlist.PathLineTo(position + float2(-framePadding.x, buttonSize.y));
			drawlist.PathFillConvex(color);
			
			drawlist.PathLineTo(position + float2(framePadding.x, 0));
			drawlist.PathLineTo(position + float2(buttonSize.x - framePadding.x, 0));
			drawlist.PathLineTo(position + float2(buttonSize.x + framePadding.x, buttonSize.y / 2));
			drawlist.PathLineTo(position + float2(buttonSize.x - framePadding.x, buttonSize.y));
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y));
			
			drawlist.PathFillConvex(color);
			
			if (haveBorder) {
				drawInnerBorder();
			}
			
			drawlist.AddText(position + framePadding * float2(2, 1),
							 ImGui::GetColorU32(ImGuiCol_Text),
							 label.data(), label.data() + label.size());
		}
		else { // Rightmost Button
			drawlist.PathLineTo(position + float2(-framePadding.x, 0));
			drawlist.PathLineTo(position + float2(framePadding.x, 0));
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y / 2));
			drawlist.PathFillConvex(color);
			
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y / 2));
			drawlist.PathLineTo(position + float2(framePadding.x, buttonSize.y));
			drawlist.PathLineTo(position + float2(-framePadding.x, buttonSize.y));
			drawlist.PathFillConvex(color);
			
			drawlist.AddRectFilled(position + float2(framePadding.x, 0),
								   position + buttonSize, color, rounding, ImDrawFlags_RoundCornersRight);
			
			drawlist.AddText(position + framePadding * float2(2, 1),
							 ImGui::GetColorU32(ImGuiCol_Text),
							 label.data(), label.data() + label.size());
		}
		
	}
	
	void DirectoryView::addressbarPopup(bool open, std::size_t directoryLineIndex, mtl::float2 position) {
		if (open) {
			ImGui::OpenPopup("addressbar-popup");
		}
		
		auto const* const directory = directoryLineIndex != 0 ? &directoryLine[directoryLineIndex - 1] : nullptr;
		std::size_t itemIndex = directory ? directory->activeIndex : 0;
		
		auto& g = *GImGui;
		auto& style = g.Style;
		
		position -= style.WindowPadding;
		position += style.FramePadding;
		position.y -= g.FontSize + style.FramePadding.y * 2;
		float const itemHeight = g.FontSize + style.ItemSpacing.y;
		position.y -= itemIndex * itemHeight;
		
		ImGui::SetNextWindowPos(position);
		if (!ImGui::BeginPopup("addressbar-popup")) {
			return;
		}
		utl_defer { ImGui::EndPopup(); };
			
		if (directoryLineIndex == 0) {
			if (addressbarPopupMenuItem(std::filesystem::directory_entry(rootDir().path))) {
				ImGui::CloseCurrentPopup();
			}
			return;
		}
		for (auto& entry: *directory) {
			if (!entry.is_directory()) {
				continue;
			}
			if (addressbarPopupMenuItem(entry)) {
				ImGui::CloseCurrentPopup();
			}
		}
	}
	
	bool DirectoryView::addressbarPopupMenuItem(std::filesystem::directory_entry const& entry) {
		poppyExpect(entry.is_directory());
		using namespace std::filesystem;
		auto const dirItr = directory_iterator(entry.path());
		bool const hasSubdirectories = dirItr != directory_iterator();
 
		if (!hasSubdirectories) {
			if (ImGui::MenuItem(entry.path().filename().c_str())) {
				gotoNextFrame(entry.path());
				return true;
			}
			return false;
		}
		
		bool pressed = false;
		if (!ImGui::BeginMenuEx(entry.path().filename().c_str(), nullptr, true, &pressed)) {
			return false;
		}
		
		utl_defer { ImGui::EndMenu(); };
		if (pressed) {
			gotoNextFrame(entry.path());
			ImGui::CloseCurrentPopup();
			return true;
		}
		
		bool result = false;
		for (auto&& subEntry: dirItr) {
			if (!subEntry.is_directory()) {
				continue;
			}
			result |= addressbarPopupMenuItem(subEntry);
		}
		return result;
	}
	
	static void displayHeader(std::string_view label, float width = 0) {
		bool const noHeader = ImGui::GetCursorPosY() == 0;
		if (noHeader) {
			ImGui::PushStyleColor(ImGuiCol_Separator, 0x0);
		}
		if (width > 0) {
			ImGui::SetNextItemWidth(width);
		}
		ImGui::Separator();
		if (noHeader) {
			ImGui::PopStyleColor();
		}
		withFont(Font::UIDefault().setWeight(FontWeight::semibold), [&]{
			disabledIf(true, [&]{
				ImGui::SetCursorPosX(GImGui->Style.WindowPadding.x);
				ImGui::TextEx(label.data(), label.data() + label.size());
			});
		});
		if (width > 0) {
			ImGui::SetNextItemWidth(width);
		}
		ImGui::Separator();
	}
	
	std::size_t DirectoryView::displayGroup(Directory& directory,
											std::size_t index,
											void (DirectoryView::*callback)(std::span<Entry>, std::size_t))
	{
		std::filesystem::path const ext = directory[index].path().extension();
		auto const firstLetter = std::toupper(directory[index].path().filename().c_str()[0]);
		
		utl::function<bool()> const breakCondition = desc.sortCondition == SortCondition::type ? utl::function([&]{
			return ext != directory[index].path().extension();
		}) : [&]{
			return firstLetter != std::toupper(directory[index].path().filename().c_str()[0]);
		};
		
		std::string const headerLabel = desc.sortCondition == SortCondition::type ?
			(ext.empty() ? "Folders" : ext.string()) :
			std::string(1, firstLetter);
		
		auto const beginIndex = index;
		for (; index < directory.size() && !breakCondition(); ++index);
		
		displayHeader(headerLabel);
		(this->*callback)({ directory.begin() + beginIndex, directory.begin() + index }, beginIndex);
		
		return index;
	}
	
	void DirectoryView::itemPopup(std::size_t index, Entry& entry, DirectoryItemDescription const& desc) {
		auto const popupID = utl::format("item-popup-{}", index);
		ImGui::OpenPopupOnItemClick(popupID.data());
		if (ImGui::BeginPopup(popupID.data())) {
			if (!entry.selected) select(index);
			utl_defer{ ImGui::EndPopup(); };
			if (ImGui::MenuItem("Open")) {
				
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Rename")) {
				entry.renaming = 2;
			}
			if (ImGui::MenuItem("Delete")) {
				deleteEntry(entry);
			}
		}
	}
	
	void DirectoryView::itemRenameField(std::size_t index, Entry& entry, mtl::float2 position, float width) {
		auto& g = *GImGui;
		auto& style = g.Style;
		
		if (entry.renaming == 2) {
			std::strncpy(renameBuffer.data(),
						 entry.path().filename().string().data(),
						 std::size(renameBuffer) - 1);
		}
		
		auto const cpSave = ImGui::GetCursorPos();
		
		ImGui::SetNextItemWidth(width);
		ImGui::SetCursorPos(position);
		if (ImGui::InputText("##rename-input", renameBuffer.data(),
							 std::size(renameBuffer) - 1,
							 ImGuiInputTextFlags_EnterReturnsTrue))
		{
			renameEntry(entry, renameBuffer.data());
			std::memset(renameBuffer.begin(), 0, renameBuffer.size());
		}
		ImGui::SetCursorPos(cpSave - ImVec2(0, style.ItemSpacing.y));

		
		if (entry.renaming == 2) {
			ImGui::SetKeyboardFocusHere(-1);
			ImGui::SetItemDefaultFocus();
			entry.renaming = 1;
		}
		else if (!ImGui::IsWindowFocused()) {
			renameEntry(entry, renameBuffer.data());
		}
	}
	
	template <std::invocable<DirectoryView::Entry> F>
	void DirectoryView::itemDragSource(Entry const& entry, F&& drawFn) {
		auto& g = *GImGui;
		auto& style = g.Style;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, style.Colors[ImGuiCol_Header]);
		if (ImGui::BeginDragDropSource()) {
			bool const havePayload = ImGui::GetDragDropPayload() && ImGui::GetDragDropPayload()->Data;
			if (!havePayload) {
				auto const payload = makeDragdropPayloadFromSelection();
				ImGui::SetDragDropPayload(FileDragDropType.data(),
										  payload.data(), payload.size(),
										  ImGuiCond_Once);
			}
			
			if (auto const* const payload = ImGui::GetDragDropPayload();
				payload && payload->DataType == FileDragDropType)
			{
				auto const path = std::filesystem::path((char const*)payload->Data);
				auto const entry = Entry{ std::filesystem::directory_entry(path) };
				
				drawFn(entry);
			}
			
			ImGui::EndDragDropSource();
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}
	
	void DirectoryView::itemDragTarget(Entry const& targetEntry) {
		if (!targetEntry.is_directory() || targetEntry.selected || !ImGui::BeginDragDropTarget()) {
			return;
		}
		utl_defer { ImGui::EndDragDropTarget(); };
		auto* const payload = ImGui::AcceptDragDropPayload(FileDragDropType.data());
		if (!payload) {
			return;
		}
		auto const paths = unloadDragdropPayload((char const*)payload->Data, payload->DataSize);
		for (auto&& path: paths) {
			if (!path.has_parent_path() || path.parent_path() == targetEntry.path()) {
				continue;
			}
			moveEntry(Entry{ std::filesystem::directory_entry(path) }, targetEntry.path());
		}
	}
	
	utl::small_vector<char> DirectoryView::makeDragdropPayloadFromSelection() const {
		utl::small_vector<std::string> selectedPaths;
		utl::small_vector<std::uint32_t> selectedIndices;
		std::size_t totalBufferSize = 0;
		for (auto&& [index, entry]: utl::enumerate(currDir())) {
			if (entry.selected) {
				selectedIndices.push_back(index);
				auto& string = selectedPaths.push_back(entry.path().string());
				totalBufferSize += string.size() + 1;
			}
		}
		utl::small_vector<char> result(totalBufferSize, utl::no_init);
		for (char* buffer = result.data(); auto& path: selectedPaths) {
			std::memcpy(buffer, path.data(), path.size() + 1);
			buffer += path.size() + 1;
		}
		return result;
	}
	
	utl::small_vector<std::filesystem::path> DirectoryView::unloadDragdropPayload(char const* data, std::size_t size) const {
		utl::small_vector<std::filesystem::path> result;
		std::size_t begin = 0, end = 0;
		for (; end < size; ++end) {
			if (data[end] == 0) {
				result.push_back(std::filesystem::path(&data[begin], &data[end]));
				begin = end + 1;
			}
		}
		return result;
	}
	
	/// MARK: TableView
	void DirectoryView::displayTableView() {
		auto& g = *GImGui;
		auto& style = g.Style; (void)style;
		
		
		if (desc.group) {
			auto& directory = currDir();
			std::size_t index = 0;
			while (index < directory.size()) {
				index = displayGroup(directory, index, &DirectoryView::displayTable);
			}
		}
		else {
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);
			displayTable(currDir());
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);
	}
	
	void DirectoryView::displayTable(std::span<Entry> entries, std::size_t beginIndex) {
		auto& g = *GImGui;
		auto& style = g.Style;
		
		float const itemWidth = 100;
		int const columnCount = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (itemWidth + style.ItemSpacing.x)));
		
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.WindowPadding.x);
		float2 const tableSize = { ImGui::GetContentRegionAvail().x - style.WindowPadding.x, 0 };
		
		ImGui::PushID(beginIndex);
		if (ImGui::BeginTable("directory", columnCount, ImGuiTableFlags_None, tableSize)) {
			utl_defer { ImGui::EndTable(); };
			for (auto&& [localIndex, entry]: utl::enumerate(entries)) {
				ImGui::TableNextColumn();
				auto const item = displayTableViewItem(itemWidth, beginIndex + localIndex, entry);
				(void)item;
			}
		}
		ImGui::PopID();
	}
	
	
	auto DirectoryView::displayTableViewItem(float declWidth, std::size_t index, Entry& entry)
		-> DirectoryView::ItemDisplayResult
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return {};

		ImGui::PushID(robin_hood::hash<std::size_t>{}(index)); // lol, hash collisions otherwise
		utl_defer { ImGui::PopID(); };
		
		float const width = ImGui::GetContentRegionAvail().x;
		
		poppyAssert(delegate != nullptr);
		DirectoryItemDescription const itemDesc = delegate->makeItemDescription(entry);
		
		auto& g = *GImGui;
		auto& style = g.Style;
		ImVec2 const itemSizeNoLabel = { width, declWidth * 0.75f };
		ImVec2 const itemSize = itemSizeNoLabel + ImVec2(0, g.FontSize + 2 * style.FramePadding.y);
		ImVec2 const pos = window->DC.CursorPos;
		entry.bb = { pos, itemSize };
		
		/// Button
		auto const result = [&]{
			ImRect const bb(pos, pos + (entry.renaming ? itemSizeNoLabel : itemSize));
			const ImGuiID id = window->GetID("item-button");
			
			ItemDisplayResult result;
			ImGui::ItemSize(itemSize, style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, id)) {
				return result;
			}
			auto buttonFlags = ImGuiButtonFlags_PressedOnDoubleClick | ImGuiButtonFlags_PressedOnDragDropHold | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnRelease;
			bool const buttonActive = ImGui::ButtonBehavior(bb, id, &result.hovered, &result.held, buttonFlags);
			if (!buttonActive) {
				return result;
			}
			if (!entry.selected || ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
				result.selected = g.IO.MouseClicked[ImGuiMouseButton_Left];
			}
			else {
				result.selected = g.IO.MouseReleased[ImGuiMouseButton_Left];
			}
			result.activated = g.IO.MouseClickedCount[ImGuiMouseButton_Left] == 2;
			result.activatedByDragDrop = !result.selected && !result.activated;
			return result;
		}();
		
		if (result.selected) {
			select(index);
		}
		
		itemDragSource(entry, [&](Entry const& entry) {
			auto const pos = ImGui::GetCurrentWindow()->DC.CursorPos;
			ImGui::ItemSize(itemSize, style.FramePadding.y);
			auto const ddItemDesc = delegate->makeItemDescription(entry);
			drawTableViewLabelAndIcon(pos, itemSize, itemSizeNoLabel,
									  ddItemDesc.label, ddItemDesc.iconName,
									  ddItemDesc.displayImage);
		});
		
		itemDragTarget(entry);
		
		/* Draw Frame */ {
			bool const selected = entry.displaySelected();
			if (result.hovered || selected) {
				auto const color = ImGui::GetColorU32(selected && result.hovered ? ImGuiCol_HeaderHovered :
													  selected ? ImGuiCol_Header :
													  ImGuiCol_FrameBgHovered);
				window->DrawList->AddRectFilled(pos, pos + itemSize,
												color, style.FrameRounding);
			}
		}
		
		if (entry.renaming) {
			itemRenameField(index, entry,
							pos + window->Scroll + ImVec2(0, itemSizeNoLabel.y) - window->Pos,
							itemSize.x);
		}
		
		drawTableViewLabelAndIcon(pos, itemSize, itemSizeNoLabel,
								  itemDesc.label, itemDesc.iconName.data(),
								  itemDesc.displayImage,
								  entry.renaming);
		
		itemPopup(index, entry, itemDesc);
		
		// open folders by drag drop
		[&]{
			if (entry.is_directory() && (result.activated || result.activatedByDragDrop)) {
				if (result.activatedByDragDrop) {
					auto* const payload = ImGui::GetDragDropPayload();
					if (!payload) { return; }
					if (payload->DataType != FileDragDropType) { return; }
					std::filesystem::path const path = (char const*)payload->Data;
					if (path == entry.path()) {
						return;
					}
				}
				gotoNextFrame(entry.path());
				return;
			}
		}();
		
		
		return result;
	}
	
	void DirectoryView::drawTableViewLabelAndIcon(mtl::float2 pos,
												  mtl::float2 size,
												  mtl::float2 sizeNoLabel,
												  std::string_view label,
												  std::string_view icon,
												  void const* image,
												  bool renaming) const
	{
		auto const iconSize = IconSize::_32;
		auto& g = *GImGui;
		auto& style = g.Style;
		auto& window = *g.CurrentWindow;
		float2 const iconSpace = { size.x, size.y - g.FontSize };
		withIconFont(iconSize, [&]{
			auto const iconText = icons.unicodeStr(std::string(icon));
			float2 const iconTextSize = ImGui::CalcTextSize(iconText.data());
			window.DrawList->AddText(pos + (iconSpace - iconTextSize) / 2, ImGui::GetColorU32(ImGuiCol_Text), iconText.data());
		});
		
		if (renaming) {
			return;
		}
		
		// Label
		float2 const labelPos = pos + float2(0, sizeNoLabel.y + style.FramePadding.y);
		
		float2 const labelSize = ImGui::CalcTextSize(label.data(), label.data() + label.size());
		float const labelWidthAvail = size.x - 2 * g.Style.FramePadding.x;
		if (labelSize.x <= labelWidthAvail) {
			window.DrawList->AddText(labelPos + float2((size.x - labelSize.x) / 2, 0),
									 ImGui::GetColorU32(ImGuiCol_Text), label.data(), label.data() + label.size());
			return;
		}
		/* Trim */ {
			std::size_t endPos = label.size();
			while (endPos > 0 && ImGui::CalcTextSize(label.data(), label.data() + endPos).x > labelWidthAvail) {
				--endPos;
			}
			std::string labelStr(label.data(), label.data() + endPos);
			for (auto&& [i, c]: utl::enumerate(utl::reverse(labelStr))) {
				if (i == 3) { break; }
				c = '.';
			}
			float2 const labelSize = ImGui::CalcTextSize(labelStr.data(), labelStr.data() + labelStr.size());
			window.DrawList->AddText(labelPos + float2((size.x - labelSize.x) / 2, 0),
									 ImGui::GetColorU32(ImGuiCol_Text), labelStr.data(), labelStr.data() + labelStr.size());
		}
		return;
	}
	
	/// MARK: Background
	static std::string toString(DirectoryView::SortCondition condition) {
		return std::array {
			"Type", "Name"
		}[utl::to_underlying(condition)];
	}
	
	void DirectoryView::displayBackground() {
		auto& g = *GImGui;
		auto& style = g.Style; (void)style;
		
		ImGui::SetCursorPos({ ImGui::GetScrollX(), ImGui::GetScrollY() });
		if (ImGui::InvisibleButton("background-button", ImGui::GetCursorPos() + ImGui::GetContentRegionAvail())) {
			if (!ImGui::IsKeyDown(ImGuiKey_ModShift) && !ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
				clearSelection();
			}
		}
		itemDragTarget(Entry{ std::filesystem::directory_entry(currentDirectory()) });
		
		backgroundContextMenu();
		if (ImGui::IsItemActivated()) {
			beginDragSelection();
		}
		updateDragSelection();
	}
	
	void DirectoryView::backgroundContextMenu() {
		ImGui::OpenPopupOnItemClick("Background Popup",
									ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup);
		if (ImGui::BeginPopup("Background Popup")) {
			if (ImGui::MenuItem("New Folder")) {
				newFolder();
			}
			if (ImGui::BeginMenu("Sort by")) {
				bool selected[utl::to_underlying(SortCondition::_count)]{};
				selected[utl::to_underlying(desc.sortCondition)] = true;
				for (auto condition: utl::enumerate<SortCondition>()) {
					if (ImGui::MenuItem(toString(condition).data(), nullptr, &selected[utl::to_underlying(condition)])) {
						setSortCondition(condition);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::MenuItem("Group", nullptr, &desc.group);
			ImGui::EndPopup();
		}
	}
	
	void DirectoryView::beginDragSelection() {
		poppyAssert(!selectionDragBegin.has_value());
		selectionDragBegin = ImGui::GetMousePos();
		if (!ImGui::IsKeyDown(ImGuiKey_ModShift) && !ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
			clearSelection();
		}
	}
	
	void DirectoryView::updateDragSelection() {
		if (!selectionDragBegin) {
			return;
		}
		auto& g = *GImGui;
		auto& window = *g.CurrentWindow;
		float2 pos = *selectionDragBegin;
		float2 size = ImGui::GetMouseDragDelta();
		
		// Normalize
		if (size.x < 0) { pos.x += size.x; size.x = -size.x; }
		if (size.y < 0) { pos.y += size.y; size.y = -size.y; }
	
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			applySelectionRect({ pos, size });
			selectionDragBegin = std::nullopt;
		}
		else {
			updateSelectionRect({ pos, size });
		}
		
		float2 max = pos + size;
		max = mtl::min(max, (float2)(ImGui::GetWindowPos() + ImGui::GetWindowSize()));
		pos = mtl::max(pos, (float2)ImGui::GetWindowPos());
		float4 rectColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
		rectColor.a *= 0.5f;
		window.DrawList->AddRectFilled(pos, max, ImGui::ColorConvertFloat4ToU32(rectColor));
		window.DrawList->AddRect(pos, max, ImGui::GetColorU32(ImGuiCol_Border));
	}
	
	/// MARK: Logic
	bool DirectoryView::setDirectory(std::filesystem::path target) {
		if (!std::filesystem::exists(target)) {
			return false;
		}
		poppyExpect(target.is_absolute());
		directoryLine.clear();
		directoryLine.resize(1); // for now
		
		auto& currentDirectory = currDir();
		currentDirectory.path = target;
		scanDirectoryLine();
		return true;
	}
	
	static auto const compareType = [](DirectoryView::Entry const& a, DirectoryView::Entry const& b) {
		if (a.is_directory()) {
			if (b.is_directory()) {
				return a.path().filename() < b.path().filename();
			}
			return true;
		}
		if (b.is_directory()) {
			return false;
		}
		return a.path().extension() < b.path().extension();
	};
	
	static auto const compareName = [](DirectoryView::Entry const& a, DirectoryView::Entry const& b) {
		return a.path().filename() < b.path().filename();
	};
	
	void DirectoryView::scanDirectory(Directory& directory, auto const& selectionBackup) const {
		directory.clear();
		for (auto&& entry: std::filesystem::directory_iterator(directory.path)) {
			if (utl::is_hidden(entry.path())) {
				continue;
			}
			auto& inserted = directory.push_back(Entry{ entry });
			if (selectionBackup.contains(inserted.path().string())) {
				inserted.selected = true;
			}
		}
		switch (desc.sortCondition) {
			case SortCondition::type:
				std::sort(directory.begin(), directory.end(), compareType);
				break;
			case SortCondition::name:
			default:
				std::sort(directory.begin(), directory.end(), compareName);
				break;
		}
	}
	
	void DirectoryView::scanDirectoryLine(bool const preserveSelection) {
		auto const& currentDirectory = currDir();
		poppyAssert(!currentDirectory.path.empty());
		
		utl::hashset<std::string> selectionBackup;
		
		if (preserveSelection) {
			for (auto const& entry: currentDirectory) {
				if (entry.selected)
					selectionBackup.insert(entry.path().string());
			}
		}
		
		utl::vector<std::filesystem::path> dirLinePaths;
		for (auto current = this->currentDirectory(); current != rootDirPath; current = current.parent_path()) {
			dirLinePaths.push_back(current);
		}
		dirLinePaths.push_back(rootDirPath);
		
		directoryLine.clear();
		directoryLine.reserve(dirLinePaths.size());
		for (auto&& [index, path]: utl::enumerate(utl::reverse(dirLinePaths))) {
			auto& dir = directoryLine.emplace_back();
			dir.path = path;
			scanDirectory(dir, selectionBackup);
			if (index > 0) { /* set active index in parent dir */
				auto& parentDir = *(directoryLine.end() - 2);
				auto const itr = std::find_if(parentDir.begin(), parentDir.end(), [&](Entry const& e) { return e.path() == dir.path; });
				poppyAssert(itr != parentDir.end());
				parentDir.activeIndex = itr - parentDir.begin();
			}
		}
	}
	
	void DirectoryView::renameEntry(Entry& entry, std::string newName) {
		renameEntry((Entry const&)entry, std::move(newName));
		entry.renaming = false;
	}
	
	void DirectoryView::renameEntry(Entry const& entry, std::string newName) {
		renameBuffer = {};
		if (!entry.renaming) {
			return;
		}
		if (newName == entry.path().filename().string()) {
			return;
		}
		if (newName.empty()) {
			return;
		}
		try {
			std::filesystem::rename(entry.path(), entry.path().parent_path() / newName);
		}
		catch (std::filesystem::filesystem_error const& e) {
			poppyLog(error, "Failed to rename file {} to '{}': {}", entry.path(), newName, e.what());
		}
		scanDirectoryLine();
	}
	
	void DirectoryView::moveEntry(Entry entry, std::filesystem::path to) {
		if (entry.path().parent_path() == to) { return; }
		dispatch(DispatchToken::nextFrame, CustomCommand([=]{
			try {
				std::filesystem::rename(entry.path(), to / entry.path().filename());
				scanDirectoryLine();
				poppyLog("Moved Entry to {}", to);
			}
			catch (std::filesystem::filesystem_error const& e) {
				poppyLog(error, "Failed to move file {}: {}", entry.path(), e.what());
			}
		}));
	}
	
	void DirectoryView::deleteEntry(Entry entry) {
		dispatch(DispatchToken::nextFrame, CustomCommand([=]{
			std::filesystem::remove_all(entry.path());
			rescan();
		}));
	}
	
	void DirectoryView::newFolder() {
		dispatch(DispatchToken::nextFrame, CustomCommand([=]{
			auto const newFolderPath = currentDirectory() / "New Folder";
			std::filesystem::create_directory(newFolderPath);
			rescan();
			auto& dir = currDir();
			auto const itr = std::find_if(dir.begin(), dir.end(), [&](Entry const& entry) { return entry.path() == newFolderPath; });
			poppyAssert(itr != dir.end());
			itr->renaming = 2;
		}));
	}
	
	void DirectoryView::gotoNextFrame(std::filesystem::path const& target) {
		if (currentDirectory() == target) {
			return;
		}
		dispatch(DispatchToken::nextFrame, CustomCommand([this, target]{
			history.push(target);
			setDirectory(target);
		}));
	}
	
	/// MARK: Selection
	void DirectoryView::clearSelection() {
		for (auto& entry: currDir()) {
			entry.selected = false;
			renameEntry(entry, renameBuffer.data());
		}
	}
	
	void DirectoryView::select(std::size_t index) {
		auto& entry = currDir()[index];
		selectEx([&](auto&& cb) {
			entry.selected = cb(entry.selected);
			renameEntry(entry, renameBuffer.data());
		});
	}

	void DirectoryView::selectEx(auto&& cb, bool clear) {
		if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
			// Union Select
			cb([](bool current) { return true; });
			return;
		}
		if (ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
			// XOR Select
			cb([](bool current) { return !current; });
			return;
		}
		// Default Select
		if (clear)
			clearSelection();
		cb([](bool current) { return true; });
	}
	
	void DirectoryView::updateSelectionRect(mtl::rectangle<float> const& bb) {
		selectEx([&](auto&& cb) {
			for (auto& entry: currDir()) {
				if (mtl::do_intersect(bb, entry.bb)) {
					entry.selectionCandidate = cb(entry.selected);
					continue;
				}
				entry.selectionCandidate = std::nullopt;
			}
			
		}, false);
	}
	
	void DirectoryView::applySelectionRect(mtl::rectangle<float> const& bb) {
		selectEx([&](auto&& cb) {
			for (auto& entry: currDir()) {
				if (mtl::do_intersect(bb, entry.bb)) {
					entry.selected = cb(entry.selected);
				}
				entry.selectionCandidate = std::nullopt;
			}
		});
	}
	
	bool DirectoryView::Entry::displaySelected() const {
		if (selectionCandidate.has_value()) {
			return selected ? selected && *selectionCandidate : *selectionCandidate;
		}
		return selected;
	}
	
}
