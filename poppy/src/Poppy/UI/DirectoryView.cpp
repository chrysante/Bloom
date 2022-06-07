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

	static std::string toString(DirectoryView::Layout layout) {
		return std::array{
			"Columns",
			"Table",
			"List"
		}[utl::to_underlying(layout)];
	}
	
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
			
			ToolbarDropdownMenu().content([=]{
				enumCombo(desc.layout);
			}).tooltip("Layout"),
			
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
		
		switch (desc.layout) {
			case Layout::columns:
				displayColumnView();
				break;
			case Layout::table:
				displayTableView();
				break;
			case Layout::list:
				// nothing here yet
				break;
			default:
				poppyDebugbreak();
				break;
		}
		
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
			scanDirectoryLine(currentDirectory());
		}
		catch (std::exception const&) {
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
		scanDirectoryLine(currentDirectory());
	}
	
	/// MARK: Private
	///
	///
	///
	/// MARK: Addressbar
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
			
			itemDragTarget(directory);
			
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
		bool const hasSubdirectories = [&]{
			for (auto& entry: dirItr) { if (entry.is_directory() && !utl::is_hidden(entry.path())) { return true; }}
			return false;
		}();
 
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
	/// MARK: Item View Generic
	///
	///
	static void displayHeader(std::string_view label, float width = 0) {
		auto& g = *GImGui;
		auto& window = *ImGui::GetCurrentWindow();
		auto& parentWindow = *window.ParentWindow;
		auto& drawlist = *window.DrawList;
		
		ImVec2 const pos = window.DC.CursorPos - ImVec2(0, 1);
		float const height = g.FontSize + 2 * g.Style.FramePadding.x;
		ImVec2 const size(window.Size.x, height);
		
		ImGui::ItemSize(size);
		drawlist.PushClipRect(window.Pos, window.Pos + ImVec2(parentWindow.Size.x, window.Size.y));
		
		mtl::float4 color = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
		color.alpha *= 0.333;
		drawlist.AddRectFilled(pos + ImVec2(-10, 0), pos + size + ImVec2(20, 0), ImGui::ColorConvertFloat4ToU32(color));
		drawlist.AddRect(pos + ImVec2(-10, 0), pos + size + ImVec2(20, 0), ImGui::GetColorU32(ImGuiCol_Separator));
		withFont(Font::UIDefault().setWeight(FontWeight::semibold), [&]{
			drawlist.AddText(pos + g.Style.FramePadding, ImGui::GetColorU32(ImGuiCol_TextDisabled), label.data(), label.data() + label.size());
		});
		
		drawlist.PopClipRect();
	}
	
	std::size_t DirectoryView::displayGroup(Directory& directory,
											std::size_t index,
											utl::function<void(std::size_t, std::size_t)> cb)
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
		cb(beginIndex, index);
		
		return index;
	}
	
	auto DirectoryView::displayItem(mtl::rectangle<float> const& itemBB,
									mtl::rectangle<float> const& labelBB,
									mtl::rectangle<float> const& iconBB,
									TextAlign textAlign,
									EntryHandle entry,
									int addFrameDrawFlags)
		-> DirectoryView::ItemButtonState
	{
		entry->bb = itemBB;
		
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return {};

		ImGui::PushID(robin_hood::hash<std::size_t>{}(entry.index())); // lol, hash collisions otherwise
		utl_defer { ImGui::PopID(); };
		
		poppyAssert(delegate != nullptr);
		DirectoryItemDescription const itemDesc = delegate->makeItemDescription(entry.get());
		
		auto& g = *GImGui;
		auto& style = g.Style;
		
		/// Button
		auto const buttonState = [&]{
			ImRect const bb = !entry->renaming ? ImRect(itemBB.lower_bound(), itemBB.upper_bound()) : ImRect(iconBB.lower_bound(), iconBB.upper_bound());
			const ImGuiID id = window->GetID("item-button");
			
			ItemButtonState buttonState{};
			ImGui::ItemSize(itemBB.size(), style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, id)) {
				return buttonState;
			}
			bool hoveredOut, heldOut;
			auto buttonFlags = ImGuiButtonFlags_PressedOnDoubleClick | ImGuiButtonFlags_PressedOnDragDropHold | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnRelease;
			bool const buttonActive = ImGui::ButtonBehavior(bb, id, &hoveredOut, &heldOut, buttonFlags);
			buttonState.hovered = hoveredOut;
			buttonState.held = heldOut;
			
			buttonState.hovered &= !g.DragDropActive || entry->is_directory();
			if (!buttonActive) {
				return buttonState;
			}
			if (!entry->selected || ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
				buttonState.selected = g.IO.MouseClicked[ImGuiMouseButton_Left];
			}
			else {
				buttonState.selected = g.IO.MouseReleased[ImGuiMouseButton_Left];
			}
			buttonState.activated = g.IO.MouseClickedCount[ImGuiMouseButton_Left] == 2;
			buttonState.activatedByDragDrop = !g.IO.MouseReleased[ImGuiMouseButton_Left] && !buttonState.selected && !buttonState.activated;
			return buttonState;
		}();
		
		itemDragSource(entry, [&](Entry const& entry, mtl::float2 position) {
			// Preview Drawing Code
			ImGui::SetCursorPos((ImVec2)position - ImGui::GetWindowPos());
			ImGui::ItemSize(itemBB.size(), style.FramePadding.y);
			auto const ddItemDesc = delegate->makeItemDescription(entry);
			
			mtl::rectangle<float> const localItemBB(position, itemBB.size());
			mtl::rectangle<float> const localLabelBB(position - itemBB.lower_bound() + labelBB.lower_bound(), labelBB.size());
			mtl::rectangle<float> const localIconBB(position - itemBB.lower_bound() + iconBB.lower_bound(), iconBB.size());
			drawItemFrame(localItemBB, ImGui::GetColorU32(ImGuiCol_Header));
			drawItem(localItemBB, localLabelBB, localIconBB, textAlign,
					 ddItemDesc.label,
					 ddItemDesc.iconName.data(),
					 ddItemDesc.displayImage);
		});
		
		itemDragTarget(entry);
		
		
		if (bool const selected = entry->displaySelected(); // Draw Frame
			buttonState.hovered || selected)
		{
			auto const color = ImGui::GetColorU32(selected && buttonState.hovered ? ImGuiCol_HeaderHovered :
												  selected ? ImGuiCol_Header :
												  ImGuiCol_FrameBgHovered);
			drawItemFrame(itemBB, color, addFrameDrawFlags);
		}
		
		
		if (entry->renaming) {
			itemRenameField(entry.index(), entry,
							(ImVec2)labelBB.lower_bound() + window->Scroll - window->Pos,
							itemBB.size().x);
		}
		
		drawItem(itemBB, labelBB, iconBB, textAlign,
				 itemDesc.label,
				 itemDesc.iconName.data(),
				 itemDesc.displayImage,
				 entry->renaming);
		
		itemPopup(entry, itemDesc);
		
		return buttonState;
	}
	
	void DirectoryView::drawItem(mtl::rectangle<float> const& bb,
								 mtl::rectangle<float> const& labelBB,
								 mtl::rectangle<float> const& iconBB,
								 TextAlign align,
								 std::string_view label,
								 std::string_view icon,
								 void const* image,
								 bool renaming) const
	{
		auto const iconSize = IconSize::_64;
		
		auto& g = *GImGui;
		auto& window = *g.CurrentWindow;
		
		[&]{
			auto const* const iconFont = icons.font(iconSize);
			if (!iconFont) { return; }
			float const size = iconBB.size().fold(utl::min);
			
			auto const iconText = icons.unicodeStr(std::string(icon));
			char const* const textBegin = iconText.data();
			char const* const textEnd = textBegin + strnlen(textBegin, iconText.size());
			float2 const iconTextSize = iconFont->CalcTextSizeA(size, FLT_MAX, -1, textBegin, textEnd, NULL);
			float2 const iconPosition = iconBB.lower_bound() + (iconBB.size() - iconTextSize) / 2;
			auto const iconColor = ImGui::GetColorU32(ImGuiCol_Text);
			window.DrawList->AddText(iconFont, size, iconPosition, iconColor, textBegin, textEnd);
		}();
		
		if (renaming) {
			return;
		}
		
		// Label
		auto const calcLabelPos = [&labelBB, align](float2 labelSize) {
			switch (align) {
				case TextAlign::left:
					return labelBB.lower_bound();
				case TextAlign::center:
					return labelBB.lower_bound() + float2((labelBB.size().x - labelSize.x) / 2, 0);
				case TextAlign::right:
					return labelBB.lower_bound() + float2(labelBB.size().x - labelSize.x, 0);
			}
		};
		
		if (float2 labelSize = ImGui::CalcTextSize(label.data(), label.data() + label.size());
			labelSize.x <= labelBB.size().x)
		{
			window.DrawList->AddText(calcLabelPos(labelSize), ImGui::GetColorU32(ImGuiCol_Text),
									 label.data(), label.data() + label.size());
		}
		else {
			std::size_t endPos = label.size();
			while (endPos > 0 && ImGui::CalcTextSize(label.data(), label.data() + endPos).x > labelBB.size().x) {
				--endPos;
			}
			std::string labelStr(label.data(), label.data() + endPos);
			for (auto&& [i, c]: utl::enumerate(utl::reverse(labelStr))) {
				if (i == 3) { break; }
				c = '.';
			}
			labelSize = ImGui::CalcTextSize(labelStr.data(), labelStr.data() + labelStr.size());
			window.DrawList->AddText(calcLabelPos(labelSize), ImGui::GetColorU32(ImGuiCol_Text),
									 labelStr.data(), labelStr.data() + labelStr.size());
		}
		return;
	}
	
	void DirectoryView::drawItemFrame(mtl::rectangle<float> const& bb,
									  std::uint32_t color,
									  int addDrawFlags) const
	{
		GImGui->CurrentWindow->DrawList->AddRectFilled(bb.lower_bound(), bb.upper_bound(),
													   color, GImGui->Style.FrameRounding, ImGuiDragDropFlags_None | addDrawFlags);
	}
	
	void DirectoryView::itemPopup(EntryHandle entry, DirectoryItemDescription const& desc) {
		auto const popupID = utl::format("item-popup-{}", index);
		ImGui::OpenPopupOnItemClick(popupID.data());
		if (ImGui::BeginPopup(popupID.data())) {
			utl_defer{ ImGui::EndPopup(); };
			if (!entry->selected) {
				select(entry);
			}
			if (ImGui::MenuItem("Open")) {
				
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Rename")) {
				entry->renaming = 2;
			}
			if (ImGui::MenuItem("Delete")) {
				deleteElement(entry->path());
			}
		}
	}
	
	void DirectoryView::itemRenameField(std::size_t index, EntryHandle entry, mtl::float2 position, float width) {
		auto& g = *GImGui;
		auto& style = g.Style;
		
		if (entry->renaming == 2) {
			std::strncpy(renameBuffer.data(),
						 entry->path().filename().string().data(),
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

		
		if (entry->renaming == 2) {
			ImGui::SetKeyboardFocusHere(-1);
			ImGui::SetItemDefaultFocus();
			entry->renaming = 1;
		}
		else if (!ImGui::IsWindowFocused()) {
			renameEntry(entry, renameBuffer.data());
		}
	}
	
	void DirectoryView::handleItemButtonState(ItemButtonState state, EntryHandle entry) {
		if (state.selected) {
			select(entry);
		}
		
		if (entry->is_directory() && (state.activated || state.activatedByDragDrop)) {
			if (state.activatedByDragDrop) {
				auto* const payload = ImGui::GetDragDropPayload();
				if (!payload) { return; }
				if (payload->DataType != FileDragDropType) { return; }
				std::filesystem::path const path = (char const*)payload->Data;
				if (path == entry->path()) {
					return;
				}
			}
			gotoNextFrame(entry->path());
			return;
		}
	}
	
	template <std::invocable<DirectoryView::Entry, mtl::float2> F>
	void DirectoryView::itemDragSource(ConstEntryHandle entry, F&& drawFn) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, 0x0);
		if (ImGui::BeginDragDropSource()) {
			bool const havePayload = ImGui::GetDragDropPayload() && ImGui::GetDragDropPayload()->Data;
			if (!havePayload) {
				auto const payload = makeDragdropPayloadFromSelection(entry.directory());
				poppyAssert(!payload.empty());
				ImGui::SetDragDropPayload(FileDragDropType.data(),
										  payload.data(), payload.size(),
										  ImGuiCond_Once);
			}
			
			if (auto const* const payload = ImGui::GetDragDropPayload();
				payload && payload->DataType == FileDragDropType)
			{
				auto const pos = ImGui::GetCurrentWindow()->DC.CursorPos;
				auto const paths = peekDragdropPayload((char const*)payload->Data, payload->DataSize);
				for (auto&& [index, path]: utl::enumerate(paths)) {
					Entry const entry{ std::filesystem::directory_entry(path) };
					drawFn(entry, pos + ImVec2(10, 10) * index);
				}
			}
			
			ImGui::EndDragDropSource();
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}
	
	void DirectoryView::itemDragTarget(Directory& directory) {
		itemDragTargetEx(directory.path);
	}
	
	void DirectoryView::itemDragTarget(ConstEntryHandle targetEntry) {
		if (!targetEntry->is_directory() || targetEntry->selected) {
			return;
		}
		itemDragTargetEx(targetEntry->path());
	}
	
	void DirectoryView::itemDragTargetEx(std::filesystem::path const& target) {
		if (!ImGui::BeginDragDropTarget()) {
			return;
		}
		utl_defer { ImGui::EndDragDropTarget(); };
		auto* const payload = ImGui::AcceptDragDropPayload(FileDragDropType.data());
		if (!payload) {
			return;
		}
		auto const paths = unloadDragdropPayload((char const*)payload->Data, payload->DataSize);
		for (auto&& path: paths) {
			if (!path.has_parent_path() || path.parent_path() == target) {
				continue;
			}
			moveElement(path, target);
		}
	}
	
	utl::small_vector<char> DirectoryView::makeDragdropPayloadFromSelection(Directory const& directory) const {
		utl::small_vector<std::string> selectedPaths;
		utl::small_vector<std::uint32_t> selectedIndices;
		std::size_t totalBufferSize = 0;
		for (auto&& [index, entry]: utl::enumerate(directory)) {
			if (!entry.selected) {
				continue;
			}
			selectedIndices.push_back(index);
			auto& string = selectedPaths.push_back(entry.path().string());
			totalBufferSize += string.size() + 1;
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
	
	utl::small_vector<std::string_view> DirectoryView::peekDragdropPayload(char const* data, std::size_t size) const {
		utl::small_vector<std::string_view> result;
		std::size_t begin = 0, end = 0;
		for (; end < size; ++end) {
			if (data[end] == 0) {
				result.push_back(std::string_view(&data[begin], end - begin));
				begin = end + 1;
			}
		}
		return result;
	}
	
	/// MARK: Column View
	///
	///
	void DirectoryView::displayColumnView() {
		auto& g = *GImGui;
		auto& style = g.Style; (void)style;
		
		auto& window = *ImGui::GetCurrentWindow();
		float2 const tableSize = { 0, window.Size.y - ImGui::GetCursorPosY() - 1 };
		
		ImGuiTableFlags tableFlags = 0;
		tableFlags |= ImGuiTableFlags_Resizable;
		tableFlags |= ImGuiTableFlags_ScrollX;
		tableFlags |= ImGuiTableFlags_SizingFixedFit;
		
		ImGui::SetCursorPosX(1);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 0, 0 }); utl_defer { ImGui::PopStyleVar(); };
		if (!ImGui::BeginTable("column-view", directoryLine.size() + 1, tableFlags, tableSize)) {
			return;
		}
		utl_defer { ImGui::EndTable(); };
		
		// Setup
		for (auto&& [index, directory]: utl::enumerate(directoryLine)) {
			ImGui::TableSetupColumn(directory.path.c_str(), ImGuiTableColumnFlags_WidthFixed, 120, window.GetID(directory.path.c_str()));
		}
		ImGui::TableSetupColumn("preview-column", ImGuiTableColumnFlags_WidthFixed, 120, window.GetID("preview-column"));
		
		// Display
		for (auto&& [index, directory]: utl::enumerate(directoryLine)) {
			ImGui::TableNextColumn();
			displayColumn(directory, index);
		}
		ImGui::TableNextColumn();
		displayPreviewColumn();
	}
	
	void DirectoryView::displayColumn(Directory& directory, std::size_t index) {
		
		ImGui::PushID(index); utl_defer { ImGui::PopID(); };
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::BeginChild("column", { 0, 0 /* to prevent mini scroll y */ }); utl_defer { ImGui::EndChild(); };
		ImGui::PopStyleVar();
		
		auto& g = *GImGui;
		auto& style = g.Style;
		
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);
		for (auto&& [index, entry]: utl::enumerate(directory)) {
			displayColumnViewItem(EntryHandle(directory, index));
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);
		
		displayBackground(directory);
	}
	
	void DirectoryView::displayPreviewColumn() {
		ImGui::BeginChild("preview-column", { 0, -2 /* to prevent mini scroll y */ }); utl_defer { ImGui::EndChild(); };
		displayEmptyWithReason("No Preview");
	}
	
	DirectoryView::ItemButtonState DirectoryView::displayColumnViewItem(EntryHandle entry) {
		auto& g = *GImGui;
		auto& style = g.Style;
		auto& window = *ImGui::GetCurrentWindow();
		float const height = g.FontSize + 2 * style.FramePadding.y;
		float const minWidth = height + 3 * style.FramePadding.x;
		float const width = std::max(minWidth, window.Size.x - style.WindowPadding.x - style.ScrollbarSize);
		
		float2 const itemSize = float2(width, height);
		float2 const padding = style.FramePadding;
		float2 const pos = window.DC.CursorPos + ImVec2(style.WindowPadding.x, 0);
		
		mtl::rectangle<float> const bb(pos, itemSize);
		mtl::rectangle<float> const labelBB(pos      + float2(height, 0) +     padding,
											itemSize - float2(height, 0) - 2 * padding);
		mtl::rectangle<float> const iconBB(pos + padding,
										    float2(height, height) - 2 * padding);
		
		ImDrawFlags frameDrawFlags = 0;
		if (entry->selected) {
			std::size_t const index = entry.index();
			Directory const& directory = entry.directory();
			bool const prevSelected = index > 0 && directory[index - 1].displaySelected();
			bool const nextSelected = index < directory.size() - 1 && directory[index + 1].displaySelected();
			if (prevSelected) { frameDrawFlags = ImDrawFlags_RoundCornersBottom; }
			if (nextSelected) { frameDrawFlags = ImDrawFlags_RoundCornersTop; }
			if (prevSelected && nextSelected) { frameDrawFlags = ImDrawFlags_RoundCornersNone; }
		}
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * ImVec2(1, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style.FramePadding + ImVec2(0, 1));
		utl_defer { ImGui::PopStyleVar(2); };
		
		auto const buttonState = displayItem(bb, labelBB, iconBB, TextAlign::left, entry, frameDrawFlags);
		handleItemButtonState(buttonState, entry);
		if (buttonState.selected && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			gotoNextFrame(entry->is_directory() ? entry->path() : entry->path().parent_path());
		}
		return buttonState;
	}
	
	/// MARK: Table View
	///
	///
	void DirectoryView::displayTableView() {
		auto& g = *GImGui;
		auto& style = g.Style; (void)style;
		
		ImGui::BeginChild("directory-view"); utl_defer { ImGui::EndChild(); };
		
		if (desc.group) {
			auto& directory = currDir();
			std::size_t index = 0;
			while (index < directory.size()) {
				index = displayGroup(directory, index, [&](std::size_t begin, std::size_t end) {
					displayTable(directory, begin, end);
				});
			}
		}
		else {
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);
			displayTable(currDir(), 0, currDir().size());
		}
		
		// add a little padding in the bottom
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);
		
		displayBackground(currDir());
	}
	
	void DirectoryView::displayTable(Directory& directory, std::size_t beginIndex, std::size_t endIndex) {
		auto& g = *GImGui;
		auto& style = g.Style;
		
		float const itemWidth = 100;
		int const columnCount = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (itemWidth + style.ItemSpacing.x)));
		
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.WindowPadding.x);
		float2 const tableSize = { ImGui::GetContentRegionAvail().x - style.WindowPadding.x, 0 };
		
		ImGui::PushID(beginIndex);
		if (ImGui::BeginTable("directory", columnCount, ImGuiTableFlags_None, tableSize)) {
			utl_defer { ImGui::EndTable(); };
			for (std::size_t i = beginIndex; i < endIndex; ++i) {
				ImGui::TableNextColumn();
				displayTableViewItem(itemWidth, EntryHandle(directory, i));
			}
		}
		ImGui::PopID();
	}
	
	
	auto DirectoryView::displayTableViewItem(float declWidth, EntryHandle entry)
		-> DirectoryView::ItemButtonState
	{
		auto& g = *GImGui;
		auto& style = g.Style;
		auto& window = *ImGui::GetCurrentWindow();
		float const width = ImGui::GetContentRegionAvail().x;
	
		float2 const itemSizeNoLabel = { width, declWidth * 0.75f };
		float2 const labelSize = float2(width, g.FontSize + 2 * style.FramePadding.y);
		float2 const itemSize = itemSizeNoLabel + float2(0, labelSize.y);
		
		float2 const padding = style.FramePadding;
		
		mtl::rectangle<float> const bb(window.DC.CursorPos, itemSize);
		mtl::rectangle<float> const labelBB(bb.lower_bound() + padding + float2(0, itemSizeNoLabel.y),
											labelSize - 2 * padding);
		mtl::rectangle<float> const iconBB(bb.lower_bound() + padding,
										   itemSizeNoLabel - 2 * padding);
		auto const buttonState = displayItem(bb, labelBB, iconBB, TextAlign::center, entry);
		handleItemButtonState(buttonState, entry);
		return buttonState;
	}
	
	/// MARK: Background
	static std::string toString(DirectoryView::SortCondition condition) {
		return std::array {
			"Type", "Name"
		}[utl::to_underlying(condition)];
	}
	
	void DirectoryView::displayBackground(Directory& directory) {
		auto& g = *GImGui;
		auto& style = g.Style; (void)style;
		
		ImGui::SetCursorPos({ ImGui::GetScrollX(), ImGui::GetScrollY() });
		if (ImGui::InvisibleButton("background-button", ImGui::GetCursorPos() + ImGui::GetContentRegionAvail())) {
			if (!ImGui::IsKeyDown(ImGuiKey_ModShift) && !ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
				clearSelection(directory);
				if (directory.path != currentDirectory()) {
					gotoNextFrame(directory.path);
				}
			}
		}
		itemDragTarget(directory);
		
		backgroundContextMenu(directory);
		if (ImGui::IsItemActivated()) {
			beginDragSelection(directory);
		}
		updateDragSelection(directory);
	}
	
	void DirectoryView::backgroundContextMenu(Directory& directory) {
		ImGui::OpenPopupOnItemClick("Background Popup",
									ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup);
		if (ImGui::BeginPopup("Background Popup")) {
			if (ImGui::MenuItem("New Folder")) {
				newFolder(directory);
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
	
	void DirectoryView::beginDragSelection(Directory& directory) {
		poppyAssert(!selectionDragState.has_value());
		std::size_t const directoryIndex = &directory - directoryLine.data();
		selectionDragState = { (float2)ImGui::GetMousePos(), directoryIndex };
		if (!ImGui::IsKeyDown(ImGuiKey_ModShift) && !ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
			clearSelection(directory);
		}
	}
	
	void DirectoryView::updateDragSelection(Directory& directory) {
		if (!selectionDragState) {
			return;
		}
		std::size_t const directoryIndex = &directory - directoryLine.begin();
		if (selectionDragState->directoryIndex != directoryIndex) {
			return;
		}
		auto& g = *GImGui;
		auto& window = *g.CurrentWindow;
		float2 pos = selectionDragState->beginPosition;
		float2 size = ImGui::GetMouseDragDelta();
		
		// Normalize
		if (size.x < 0) { pos.x += size.x; size.x = -size.x; }
		if (size.y < 0) { pos.y += size.y; size.y = -size.y; }
	
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			applySelectionRect(directory, { pos, size });
			selectionDragState = std::nullopt;
		}
		else {
			updateSelectionRect(directory, { pos, size });
		}
		
		ImRect const windowRect = window.Rect();
		
		
		
		float2 max = pos + size;
		max = mtl::min(max, (float2)windowRect.Max);
		pos = mtl::max(pos, (float2)windowRect.Min);
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
		scanDirectoryLine(target);
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
	
	void DirectoryView::scanDirectoryLine(std::filesystem::path leafDirectory, bool const preserveSelection) {
		poppyExpect(!leafDirectory.empty());
		if (!std::filesystem::is_directory(leafDirectory)) {
			throw std::runtime_error("Path is not a Directory");
		}
		utl::hashset<std::string> selectionBackup;
		
		if (preserveSelection) {
			for (auto const& directory: directoryLine) {
				for (auto const& entry: directory) {
					if (entry.selected) {
						selectionBackup.insert(entry.path().string());
					}
				}
			}
		}
		
		utl::small_vector<std::filesystem::path, 8> dirLinePaths;
		for (auto current = leafDirectory; current != rootDirPath; current = current.parent_path()) {
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
	
	void DirectoryView::renameEntry(EntryHandle entry, std::string newName) {
		renameEntry(ConstEntryHandle(entry), std::move(newName));
		entry->renaming = false;
	}
	
	void DirectoryView::renameEntry(ConstEntryHandle entry, std::string newName) {
		renameBuffer = {};
		if (!entry->renaming) {
			return;
		}
		if (newName == entry->path().filename().string()) {
			return;
		}
		if (newName.empty()) {
			return;
		}
		try {
			std::filesystem::rename(entry->path(), entry->path().parent_path() / newName);
		}
		catch (std::filesystem::filesystem_error const& e) {
			poppyLog(error, "Failed to rename file {} to '{}': {}", entry->path(), newName, e.what());
		}
		scanDirectoryLine(currentDirectory());
	}
	
	void DirectoryView::moveElement(std::filesystem::path element, std::filesystem::path to) {
		poppyExpect(std::filesystem::is_directory(to));
		if (element.parent_path() == to) { return; }
		dispatch(DispatchToken::nextFrame, CustomCommand([=]{
			try {
				std::filesystem::rename(element, to / element.filename());
				rescan();
				poppyLog("Moved Entry to {}", to);
			}
			catch (std::filesystem::filesystem_error const& e) {
				poppyLog(error, "Failed to move file {}: {}", element, e.what());
			}
		}));
	}
	
	void DirectoryView::deleteElement(std::filesystem::path element) {
		dispatch(DispatchToken::nextFrame, CustomCommand([=]{
			std::filesystem::remove_all(element);
			rescan();
		}));
	}
	
	void DirectoryView::newFolder(Directory& directory) {
		dispatch(DispatchToken::nextFrame, CustomCommand([this, &directory]{
			auto const newFolderPath = directory.path / "New Folder";
			std::filesystem::create_directory(newFolderPath);
			rescan();
			auto const itr = std::find_if(directory.begin(), directory.end(), [&](Entry const& entry) {
				return entry.path() == newFolderPath;
			});
			poppyAssert(itr != directory.end());
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
	void DirectoryView::clearSelection(Directory& directory) {
		for (auto&& [index, entry]: utl::enumerate(directory)) {
			entry.selected = false;
			renameEntry(ConstEntryHandle(directory, index), renameBuffer.data());
		}
	}
	
	void DirectoryView::select(EntryHandle entry) {
		selectEx(entry.directory(), [&](auto&& cb) {
			entry->selected = cb(entry->selected);
			renameEntry(entry, renameBuffer.data());
		});
	}

	void DirectoryView::selectEx(Directory& directory, auto&& cb, bool clear) {
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
			clearSelection(directory);
		cb([](bool current) { return true; });
	}
	
	void DirectoryView::updateSelectionRect(Directory& directory, mtl::rectangle<float> const& bb) {
		selectEx(directory, [&](auto&& cb) {
			for (auto& entry: directory) {
				if (mtl::do_intersect(bb, entry.bb)) {
					entry.selectionCandidate = cb(entry.selected);
					continue;
				}
				entry.selectionCandidate = std::nullopt;
			}
			
		}, false);
	}
	
	void DirectoryView::applySelectionRect(Directory& directory, mtl::rectangle<float> const& bb) {
		selectEx(directory, [&](auto&& cb) {
			for (auto& entry: directory) {
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

