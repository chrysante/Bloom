#define UTL_DEFER_MACROS

#include "Toolbar.hpp"

#include <algorithm>
#include <numeric>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <utl/scope_guard.hpp>

#include "Poppy/Debug.hpp"
#include "Poppy/IconConfig.hpp"
#include "Poppy/ImGui/ImGui.hpp"

using namespace mtl::short_types;

namespace poppy {
    
	ToolbarButton::ToolbarButton(std::string label, utl::function<void()> block):
		label(std::move(label)),
		block(std::move(block))
	{
		
	}
	
	ToolbarIconButton::ToolbarIconButton(char const* text, utl::function<void()> block):
		icon([=]{ return text; }),
		block(std::move(block))
	{
		
	}
	
	ToolbarIconButton::ToolbarIconButton(utl::function<char const*()> icon,
										 utl::function<void()> block):
		icon(std::move(icon)),
		block(std::move(block))
	{
		
	}
	
	ToolbarIconButton::ToolbarIconButton(utl::function<char const*()> icon,
										 utl::function<bool()> enabled,
										 utl::function<void()> block):
		icon(std::move(icon)),
		block(std::move(block)),
		enabled(std::move(enabled))
	{
		
	}
	
	ToolbarIconButton::ToolbarIconButton(utl::function<void()> block,
										 utl::function<char const*()> icon,
										 utl::function<char const*()> tooltip,
										 utl::function<bool()> enabled):
		block(std::move(block)),
		icon(std::move(icon)),
		tooltip(std::move(tooltip)),
		enabled(std::move(enabled))
	{}
	
	void ToolbarItem::calcWidth(float height) {
		switch (type()) {
			case Type::button: {
				float2 const textSize = withFont(FontWeight::semibold, FontStyle::roman, [&]{
					return ImGui::CalcTextSize(get<Type::button>().label.data());
				});
				
				width = textSize.x + 6 * GImGui->Style.FramePadding.y;
				break;
			}
			case Type::iconButton: {
				width = (int)(height * 1.5f);
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
			default:
				width = 0;
				break;
		}
	}

	/// MARK: - Toolbar
	Toolbar::Toolbar() {
		
	}
	
	Toolbar::Toolbar(std::initializer_list<ToolbarItem> list):
		items(std::move(list))
	{
		
	}
	
	void Toolbar::addItem(ToolbarItem item) {
		items.push_back(std::move(item));
		cooked = false;
	}
	
	float Toolbar::getWidthWithoutSpacers() const {
		return std::accumulate(blocks.begin(), blocks.end(), 0.0f, [](float acc, Block const& block) {
			return acc + block.width;
		}) + (blocks.size() - 1) * GImGui->Style.ItemSpacing.x;
	}
	
	void Toolbar::display(float width) {
		float const newHeight = std::min(ImGui::GetContentRegionAvail().y, maxHeight);
		cooked &= height == newHeight;
		this->height = newHeight;
		
		if (!cooked) {
			cook();
		}
		
		position = ImGui::GetCursorPos();
		
		
		float const totalWidth = width != 0.0f ? width : ImGui::GetContentRegionAvail().x;
		calcOffsets(totalWidth);
		for (auto const& block: blocks) {
			if (!block.visible) {
				continue;
			}
			displayBlock(block);
		}
		
		ImGui::SetCursorPos(position + float2(0, height));
	}
	
	void Toolbar::displayBlock(Block const& block) {
		ImGui::SetCursorPos(position + float2(block.offset, 0));
		for (std::size_t i = block.itemBeginIndex; i < block.itemEndIndex; ++i) {
			displayItem(items[i], i);
		}
	}
	
	static bool toolbarStyleButtonEx(char const* label, std::size_t id, mtl::float2 size, bool enabled = true) {
		ImGui::BeginDisabled(!enabled);
		ImGui::PushStyleColor(ImGuiCol_Button, 0);
		float4 hovered = GImGui->Style.Colors[ImGuiCol_ButtonHovered];
		hovered.a /= 2;
		float4 active = GImGui->Style.Colors[ImGuiCol_ButtonActive];
		active.a /= 2;
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
		ImGui::PushID((int)id);
		bool const result = ImGui::Button(label, size);
		ImGui::PopID();
		ImGui::PopStyleColor(3);
		ImGui::EndDisabled();
		return result;
	}
	
	static bool toolbarStyleButton(char const* label, std::size_t id, mtl::float2 size, bool enabled = true) {
		return withFont(FontWeight::semibold, FontStyle::roman, [&]{
			return toolbarStyleButtonEx(label, id, size, enabled);
		});
	}
	
	static bool toolbarStyleIconButton(char const* icon, std::size_t id, mtl::float2 size, bool enabled = true) {
		ImGui::PushFont((ImFont*)IconConfig::font(16));
		bool const result = toolbarStyleButtonEx(IconConfig::unicodeStr(icon).data(), id, size, enabled);
		ImGui::PopFont();
		return result;
	}
	
	static bool toolbarStyleIconButton(char const* icon,
									   std::size_t id,
									   mtl::float2 size,
									   char const* tooltip = nullptr,
									   bool enabled = true)
	{
		ImGui::PushFont((ImFont*)IconConfig::font(16));
		bool const result = toolbarStyleButtonEx(IconConfig::unicodeStr(icon).data(), id, size, enabled);
		ImGui::PopFont();
		
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
	
	void Toolbar::displayItem(ToolbarItem const& item, std::size_t index) {
		switch (item.type()) {
			case ToolbarItem::Type::button: {
				auto const& button = item.get<ToolbarItem::Type::button>();
				if (toolbarStyleButton(button.label.data(), index, { item.width, height })) {
					button.block();
				}
				break;
			}

			case ToolbarItem::Type::iconButton: {
				auto const& button = item.get<ToolbarItem::Type::iconButton>();
				char const* const tooltip = button.tooltip ? button.tooltip() : nullptr;
				bool const enabled = button.enabled == nullptr || button.enabled();
				if (toolbarStyleIconButton(button.icon(), index,
										   { item.width, height },
										   tooltip,
										   enabled))
				{
					button.block();
				}
				break;
			}
				
			default:
				break;
		}
		
		ImGui::SameLine();
	}
	
	void Toolbar::cook() {
		blocks.clear();
		cooked = true;
		
		if (items.empty()) {
			return;
		}
		
		// sort into blocks
		[this]{
			blocks.emplace_back();
			blocks.back().itemBeginIndex = 0;
			
			for (std::size_t index = 0;
				 auto& item: items)
			{
				utl_defer {
					++index;
				};
				
				if (item.type() == ToolbarItem::Type::spacer) {
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
			item.calcWidth(height);
		}
		
		// calcs block widths
		for (auto& block: blocks) {
			block.width = std::accumulate(items.begin() + block.itemBeginIndex,
										  items.begin() + block.itemEndIndex, 0.0f,
										  [](float acc, ToolbarItem const& item) {
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
		
		if (blocks.size() == 1) {
			return;
		}
		
		cursor += blocks[0].width;
		widthAvail -= blocks[0].width;
		
		auto& lastBlock = blocks.back();
		
		if (lastBlock.width > widthAvail) {
			return;
		}
		
		lastBlock.visible = true;
		
		lastBlock.offset = totalWidth - lastBlock.width;
		
		// ...
	}
	
}
