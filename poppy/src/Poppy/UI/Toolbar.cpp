#define UTL_DEFER_MACROS
#define IMGUI_DEFINE_MATH_OPERATORS

#include "Toolbar.hpp"

#include <algorithm>
#include <numeric>
#include <utl/UUID.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <utl/scope_guard.hpp>

#include "Poppy/Core/Debug.hpp"
#include "Poppy/UI/ImGuiHelpers.hpp"

using namespace mtl::short_types;

namespace poppy {

	ToolbarDropdownMenu::ToolbarDropdownMenu():
		_id(utl::UUID::generate().to_string())
	{}
	
	void ToolbarItemUnion::calcWidth(float height) {
		switch (type()) {
			case Type::button: {
				float2 const textSize = withFont(FontWeight::semibold, FontStyle::roman, [&]{
					return ImGui::CalcTextSize(get<Type::button>()._label.data());
				});
				
				width = textSize.x + 6 * GImGui->Style.FramePadding.y;
				break;
			}
			case Type::iconButton: {
				width = (int)(height * 1.5f);
				break;
			}
			case Type::dropdownMenu: {
				auto& ddMenu = get<Type::dropdownMenu>();
				if (ddMenu._preview) {
					float2 const textSize = withFont(FontWeight::semibold, FontStyle::roman, [&]{
						return ImGui::CalcTextSize(get<Type::dropdownMenu>()._preview().data());
					});
					
					width = textSize.x + 6 * GImGui->Style.FramePadding.y;
				}
				else {
					width = (int)(height * 1.5f);
				}
				break;
			}
			case Type::separator: {
				width = 0;
				break;
			}
			case Type::spacer: {
				width = 0;
				break;
			}
			case Type::customElement:
				width = get<Type::customElement>()._width;
			default:
				width = 0;
				break;
		}
	}

	/// MARK: - Toolbar
	Toolbar::Toolbar() {
		
	}
	
	Toolbar::Toolbar(std::initializer_list<ToolbarItemUnion> list):
		items(std::move(list))
	{
		
	}
	
	void Toolbar::addItem(ToolbarItemUnion item) {
		items.push_back(std::move(item));
		cooked = false;
	}
	
	float Toolbar::getWidthWithoutSpacers() const {
		if (blocks.empty()) {
			return 0;
		}
		return std::accumulate(blocks.begin(), blocks.end(), 0.0f, [](float acc, Block const& block) {
			return acc + block.width;
		}) + (blocks.size() - 1) * GImGui->Style.ItemSpacing.x;
	}
	
	void Toolbar::display(float width) {
		float const newHeight = std::min(ImGui::GetContentRegionAvail().y, style.height);
		cooked &= actualHeight == newHeight;
		this->actualHeight = newHeight;
		
//		if (!cooked) {
			cook(!cooked);
//		}
		
		position = ImGui::GetCursorPos();
		
		// display blocks
		float const totalWidth = width != 0.0f ? width : ImGui::GetContentRegionAvail().x;
		calcOffsets(totalWidth);
		for (auto const& block: blocks) {
			if (!block.visible) {
				continue;
			}
			displayBlock(block);
		}
	}
	
	void Toolbar::displayBlock(Block const& block) {
		ImGui::SetCursorPos(position + float2(block.offset, 0));
		for (std::size_t i = block.itemBeginIndex; i < block.itemEndIndex; ++i) {
			displayItem(items[i], i);
		}
	}
	
	void Toolbar::displayItem(ToolbarItemUnion const& item, std::size_t index) {
		switch (item.type()) {
			case ToolbarItemUnion::Type::button: {
				auto const& buttonData = item.get<ToolbarItemUnion::Type::button>();
				if (button(buttonData._label.data(), index,
						   { item.width, actualHeight }, buttonData._addButtonFlags))
				{
					if (buttonData._block) {
						buttonData._block();
					}
				}
				break;
			}

			case ToolbarItemUnion::Type::iconButton: {
				auto const& buttonData = item.get<ToolbarItemUnion::Type::iconButton>();
				char const* const tooltip = buttonData._tooltip ? buttonData._tooltip() : nullptr;
				bool const enabled = buttonData._enabled == nullptr || buttonData._enabled();
				if (iconButton(buttonData._icon(), index,
							   { item.width, actualHeight },
							   buttonData._addButtonFlags,
							   tooltip,
							   enabled))
				{
					if (buttonData._block) {
						buttonData._block();
					}
				}
				break;
			}
				
			case ToolbarItemUnion::Type::dropdownMenu: {
				auto const& menuData = item.get<ToolbarItemUnion::Type::dropdownMenu>();
				
				if (beginCombo(menuData, index, { item.width, actualHeight })) {
					if (menuData._content) {
						menuData._content();
					}
					ImGui::EndCombo();
				}
				break;
			}
				
			case ToolbarItemUnion::Type::customElement: {
				auto const& customItem = item.get<ToolbarItemUnion::Type::customElement>();
				customItem._block(float2(item.width, actualHeight));
				break;
			}
				
			default:
				break;
		}
		ImGui::SameLine();
	}
	
	/// MARK: Buttons
	bool Toolbar::buttonEx(char const* label, std::size_t id, mtl::float2 size,
						   ImGuiButtonFlags addFlags, bool enabled) const
	{
		ImGui::BeginDisabled(!enabled);
		
		float4 color = GImGui->Style.Colors[ImGuiCol_Button];
		color.a *= style.buttonAlpha;
		float4 colorHovered = GImGui->Style.Colors[ImGuiCol_ButtonHovered];
		colorHovered.a *= style.buttonAlphaHovered;
		float4 colorActive = GImGui->Style.Colors[ImGuiCol_ButtonActive];
		colorActive.a *= style.buttonAlphaActive;
		
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorHovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorActive);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		ImGui::PushID((int)id);
		bool const result = ImGui::ButtonEx(label, size, ImGuiButtonFlags_PressedOnDefault_ | addFlags);
		ImGui::PopID();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
		ImGui::EndDisabled();
		return result;
	}
	
	bool Toolbar::button(char const* label, std::size_t id, mtl::float2 size,
						 ImGuiButtonFlags addFlags, bool enabled) const {
		return withFont(FontWeight::semibold, FontStyle::roman, [&]{
			return buttonEx(label, id, size, enabled);
		});
	}
	
	bool Toolbar::iconButton(char const* icon,
							 std::size_t id,
							 mtl::float2 size,
							 ImGuiButtonFlags addFlags,
							 char const* tooltip,
							 bool enabled) const
	{
		bool const result = withIconFont(IconSize::_16, [&]{
			return buttonEx(icons.unicodeStr(icon).data(), id, size, addFlags, enabled);
		});
		
		static float tooltipTimer = 1;
		
		if (tooltip && ImGui::IsItemHovered()) {
			tooltipTimer -= GImGui->IO.DeltaTime;
			
			if (tooltipTimer <= 0) {
				ImGui::BeginTooltip();
				ImGui::Text("%s", tooltip);
				ImGui::EndTooltip();
			}
		}
		else if (!ImGui::IsAnyItemHovered()) {
			tooltipTimer = 1;
		}
		
		return result;
	}
	
	bool Toolbar::beginCombo(ToolbarDropdownMenu const& menuData, std::size_t index, mtl::float2 size) const {
		char const* const tooltip = menuData._tooltip ? menuData._tooltip() : nullptr;
		bool const enabled = menuData._enabled == nullptr || menuData._enabled();
		using namespace ImGui;
		ImGuiWindow* window = GetCurrentWindow();
		
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size.x, actualHeight + GImGui->Style.ItemSpacing.y));
		
		bool pressed = false;
		
		if (menuData._preview) {
			pressed = button(menuData._preview().data(),
							 index,
							 size,
							 0,
							 enabled);
		}
		else {
			pressed = iconButton(menuData._icon ? menuData._icon() : "down-open",
								 index,
								 size,
								 0,
								 tooltip,
								 enabled);
		}
		
		
		const ImGuiID id = window->GetID(menuData._id.data());
		
		const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
		bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
		if (pressed && !popup_open)
		{
			OpenPopupEx(popup_id, ImGuiPopupFlags_None);
			popup_open = true;
		}
		
		if (!popup_open)
			return false;
		
		SetNextWindowSizeConstraints(menuData._minSize, { FLT_MAX, FLT_MAX });
		
		auto& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2 * style.FramePadding.x, style.FramePadding.y));
		bool result =  ImGui::BeginComboPopup(popup_id,
											  bb, ImGuiComboFlags_None);
		ImGui::PopStyleVar();
		return result;
	}
	
	/// MARK: Cooking
	void Toolbar::cook(bool full) {
		blocks.clear();
		cooked = true;
		
		if (items.empty()) {
			return;
		}
		
		// sort into blocks
		[this, full]{
//			if (!full) { return; }
			
			blocks.emplace_back();
			blocks.back().itemBeginIndex = 0;
			
			for (std::size_t index = 0;
				 auto& item: items)
			{
				utl_defer {
					++index;
				};
				
				bool isSpacer = item.type() == ToolbarItemUnion::Type::spacer;
				isSpacer |= item.type() == ToolbarItemUnion::Type::customElement && item.get<ToolbarItemUnion::Type::customElement>()._width == 0;
				if (isSpacer) {
					blocks.back().itemEndIndex = index + 1;
					blocks.emplace_back();
					blocks.back().itemBeginIndex = index + 1;
					continue;
				}
			}
			
			blocks.back().itemEndIndex = items.size();
		}();
		
		// calc item widths
		for (auto& item: items) {
			item.calcWidth(actualHeight);
		}
		
		// calcs block widths
		for (auto& block: blocks) {
			block.width = std::accumulate(items.begin() + block.itemBeginIndex,
										  items.begin() + block.itemEndIndex, 0.0f,
										  [](float acc, ToolbarItemUnion const& item) {
				return acc + item.width;
			});
			block.width += (block.itemEndIndex - block.itemBeginIndex - 1) * GImGui->Style.ItemSpacing.x;
		}
		
	}
	
	void Toolbar::calcOffsets(float const totalWidth) {
		poppyExpect(cooked);
		
		for (auto& block: blocks) { block.visible = false; }
		
		if (blocks.size() == 0) {
			return;
		}
		float cursor = 0;
		float widthAvail = totalWidth;
		
		blocks[0].offset = cursor;
		blocks[0].visible = true;
		auto const setBlock0CustomWidth = [&](float width){
			auto& block0LastElement = items[blocks[0].itemEndIndex - 1];
			if (block0LastElement.type() == ToolbarItemUnion::Type::customElement && block0LastElement.width == 0) {
				block0LastElement.width = width;			}
		};
		
		cursor += blocks[0].width;
		widthAvail -= blocks[0].width;
		
		if (blocks.size() == 1) {
			setBlock0CustomWidth(widthAvail);
			return;
		}
		
		auto& lastBlock = blocks.back();
		
		if (lastBlock.width > widthAvail) {
			setBlock0CustomWidth(widthAvail);
			return;
		}
		
		lastBlock.visible = true;
		
		lastBlock.offset = totalWidth - lastBlock.width;
		
		widthAvail -= lastBlock.width;
		
		if (blocks.size() == 2) {
			setBlock0CustomWidth(widthAvail);
			return;
		}

		if (blocks[1].width > widthAvail) {
			return;
		}
		
		blocks[1].visible = true;
		blocks[1].offset = cursor + (widthAvail - blocks[1].width) / 2;
		return;
		// ...
	}
	
}
