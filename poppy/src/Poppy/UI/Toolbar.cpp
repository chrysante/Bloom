#include "Poppy/UI/Toolbar.h"

#include <algorithm>
#include <numeric>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/view.hpp>
#include <utl/uuid.hpp>

#include "Poppy/Core/Debug.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace vml::short_types;
using namespace poppy;
using namespace ranges::views;

static bool tbButtonImpl(ToolbarStyle const& style, char const* label, int ID,
                         float2 size, bool enabled, bool open) {
    ImGui::BeginDisabled(!enabled);
    float4 color =
        GImGui->Style.Colors[open ? ImGuiCol_ButtonActive : ImGuiCol_Button];
    color.a *= style.buttonAlpha;
    float4 colorHovered =
        GImGui->Style
            .Colors[open ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered];
    colorHovered.a *= style.buttonAlphaHovered;
    float4 colorActive = GImGui->Style.Colors[ImGuiCol_ButtonActive];
    colorActive.a *= style.buttonAlphaActive;
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorHovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorActive);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    ImGui::PushID(ID);
    bool result = ImGui::Button(label, size);
    ImGui::PopID();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    ImGui::EndDisabled();
    return result;
}

bool tbButton(ToolbarStyle const& style, char const* label, int ID, float2 size,
              bool enabled = true, bool open = false) {
    return withFont(FontWeight::Semibold, FontStyle::Roman, [&] {
        return tbButtonImpl(style, label, ID, size, enabled, open);
    });
}

bool tbIconButton(ToolbarStyle const& style, IconData icon, int ID, float2 size,
                  char const* tooltip, bool enabled = true, bool open = false) {
    bool result = withFont(icon.font, [&] {
        return tbButtonImpl(style, icon.utf8Rep.data(), ID, size, enabled,
                            open);
    });
    static float tooltipTimer = 1.0f;
    if (tooltip && ImGui::IsItemHovered()) {
        tooltipTimer -= GImGui->IO.DeltaTime;
        if (tooltipTimer <= 0.0f) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", tooltip);
            ImGui::EndTooltip();
        }
    }
    else if (!ImGui::IsAnyItemHovered()) {
        tooltipTimer = 1.0f;
    }
    return result;
}

void ToolbarButton::display(ToolbarStyle const& style) {
    if (tbButton(style, _label.c_str(), ID(),
                 float2(getWidth(), style.actualHeight)) &&
        _action)
    {
        _action();
    }
}

float ToolbarButton::calcWidth(float /* height */) const {
    float2 textSize = withFont(FontWeight::Semibold, FontStyle::Roman, [&] {
        return ImGui::CalcTextSize(_label.data());
    });
    return textSize.x + 6 * GImGui->Style.FramePadding.y;
}

void ToolbarIconButton::display(ToolbarStyle const& style) {
    if (tbIconButton(style, FontManager::get({ IconSize::_16 }, _icon()), ID(),
                     float2(getWidth(), style.actualHeight), getTooltip(),
                     getIsEnabled()) &&
        _action)
    {
        _action();
    }
}

float ToolbarIconButton::calcWidth(float height) const { return height * 1.5f; }

ToolbarDropdownMenu::ToolbarDropdownMenu():
    _id(utl::uuid::generate().to_string()) {}

bool ToolbarDropdownMenu::beginCombo(ToolbarStyle const& style) {
    using namespace ImGui;
    float2 size = { getWidth(), style.actualHeight };
    ImGuiWindow* window = GetCurrentWindow();
    ImRect bb(window->DC.CursorPos,
              (float2)window->DC.CursorPos +
                  float2(size.x,
                         style.actualHeight + GImGui->Style.ItemSpacing.y));
    ImGuiID id = window->GetID(_id.data());
    ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    bool pressed = false;
    if (_preview) {
        pressed = tbButton(style, _preview().c_str(), ID(), size,
                           getIsEnabled(), popup_open);
    }
    else {
        char const* icon = _icon ? _icon() : "down-open";
        pressed = tbIconButton(style, FontManager::get({ IconSize::_16 }, icon),
                               ID(), size, getTooltip(), getIsEnabled(),
                               popup_open);
    }
    if (pressed && !popup_open) {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }
    if (!popup_open) {
        return false;
    }
    SetNextWindowSizeConstraints(_minSize, { FLT_MAX, FLT_MAX });
    PushStyleVar(ImGuiStyleVar_FramePadding,
                 float2(2, 1) * (float2)GetStyle().FramePadding);
    bool result = BeginComboPopup(popup_id, bb, ImGuiComboFlags_None);
    PopStyleVar();
    return result;
}

void ToolbarDropdownMenu::display(ToolbarStyle const& style) {
    if (beginCombo(style)) {
        if (_content) {
            _content();
        }
        ImGui::EndCombo();
    }
}

float ToolbarDropdownMenu::calcWidth(float height) const {
    if (!_preview) {
        return height * 1.5f;
    }
    float2 textSize = withFont(FontWeight::Semibold, FontStyle::Roman, [&] {
        return ImGui::CalcTextSize(_preview().data());
    });
    return textSize.x + 6 * GImGui->Style.FramePadding.y;
}

void ToolbarSeparator::display(ToolbarStyle const&) {}

float ToolbarSeparator::calcWidth([[maybe_unused]] float height) const {
    return 0.0f;
}

void ToolbarSpacer::display(ToolbarStyle const&) {}

float ToolbarSpacer::calcWidth([[maybe_unused]] float height) const {
    return 0.0f;
}

void ToolbarEmptyItem::display(ToolbarStyle const& style) {
    ImGui::Dummy(float2(getWidth(), style.actualHeight));
}

float ToolbarEmptyItem::calcWidth([[maybe_unused]] float height) const {
    return _width;
}

float Toolbar::getWidthWithoutSpacers() const {
    if (blocks.empty()) {
        return 0;
    }
    return std::accumulate(blocks.begin(), blocks.end(), 0.0f,
                           [](float acc, Block const& block) {
        return acc + block.width;
    }) + (blocks.size() - 1) * GImGui->Style.ItemSpacing.x;
}

void Toolbar::display(std::optional<float> width) {
    float newHeight = std::min(ImGui::GetContentRegionAvail().y, style.height);
    cooked &= style.actualHeight == newHeight;
    style.actualHeight = newHeight;
    /// We actually cook every frame because performance is really not an issue
    /// and it solves some caching issues
    cook();
    float2 position = ImGui::GetCursorPos();
    /// Display blocks
    if (!width) {
        width = ImGui::GetContentRegionAvail().x;
    }
    calcOffsets(*width);
    for (auto const& block: blocks) {
        if (block.visible) {
            ImGui::SetCursorPos(position + float2(block.offset, 0));
            displayBlock(block);
        }
    }
    ImGui::SetCursorPos(position + float2(0, style.actualHeight));
}

void Toolbar::displayBlock(Block const& block) {
    for (size_t i = block.itemBeginIndex; i < block.itemEndIndex; ++i) {
        items[i]->display(style);
        ImGui::SameLine();
    }
}

void Toolbar::cook() {
    blocks.clear();
    cooked = true;
    if (items.empty()) {
        return;
    }
    /// Partition into blocks
    blocks.emplace_back();
    blocks.back().itemBeginIndex = 0;
    for (auto [index, item]: items | enumerate) {
        if (dynamic_cast<ToolbarSpacer const*>(item.get())) {
            blocks.back().itemEndIndex = index + 1;
            blocks.emplace_back();
            blocks.back().itemBeginIndex = index + 1;
            continue;
        }
    }
    blocks.back().itemEndIndex = items.size();
    /// Calculate item widths
    for (auto& item: items) {
        item->_width = item->calcWidth(style.actualHeight);
    }
    /// Calculate block widths
    for (auto& block: blocks) {
        block.width = std::accumulate(items.begin() + block.itemBeginIndex,
                                      items.begin() + block.itemEndIndex, 0.0f,
                                      [](float acc, auto& item) {
            return acc + item->getWidth();
        });
        block.width += (block.itemEndIndex - block.itemBeginIndex - 1) *
                       GImGui->Style.ItemSpacing.x;
    }
}

void Toolbar::calcOffsets(float totalWidth) {
    assert(cooked);
    for (auto& block: blocks) {
        block.visible = false;
    }
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
    widthAvail -= lastBlock.width;
    if (blocks.size() == 2) {
        return;
    }
    if (blocks[1].width > widthAvail) {
        return;
    }
    blocks[1].visible = true;
    blocks[1].offset = cursor + (widthAvail - blocks[1].width) / 2;
}
