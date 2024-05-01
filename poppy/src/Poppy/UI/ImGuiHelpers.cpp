#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Poppy/Core/Debug.h"

using namespace poppy;
using namespace vml::short_types;

ViewportInput poppy::detectViewportInput(ImGuiButtonFlags buttonFlags) {
    buttonFlags |= ImGuiButtonFlags_NoNavFocus;
    // Save to restore later
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGuiID hovered = ImGui::GetCurrentContext()->HoveredId;
    ImGui::SetCursorPos({ 0, 0 });
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return ViewportInput{};
    }
    ImGuiID id = window->GetID("InvisibleButton");
    ImVec2 size = ImGui::CalcItemSize(window->Size, 0.0f, 0.0f);
    ImRect bb(window->DC.CursorPos,
              float2(window->DC.CursorPos) + float2(size));
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) return {};
    ViewportInput result{};
    result.pressed = ImGui::ButtonBehavior(bb, id, &result.hovered,
                                           &result.held, buttonFlags);
    ImGui::SetHoveredID(hovered);
    ImGui::SetCursorPos(cursorPos);
    return result;
}

bool poppy::dragFloat3Pretty(char const* label, float v[3], float v_speed,
                             float v_min, float v_max, char const* format,
                             ImGuiSliderFlags flags) {
    int const components = 3;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    ImGuiContext& g = *GImGui;
    auto& style = ImGui::GetStyle();
    bool value_changed = false;
    float height = ImGui::GetFontSize() + 2 * style.FramePadding.y;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    constexpr std::array colors = { ImU32(0xFF1111FFu), ImU32(0xFF00CC00u),
                                    ImU32(0xFFFF2222u) };
    constexpr std::array labels = { "X", "Y", "Z" };
    ImVec2 labelButtonSize = { height * 1.5f, height };
    float fullWidth = ImGui::CalcItemWidth();
    bool haveLabelButtons = fullWidth / components >= 90;
    float itemWidth =
        fullWidth / components - (haveLabelButtons ? labelButtonSize.x : 0);
    auto beginCursor = window->DC.CursorPos;
    auto* DL = ImGui::GetWindowDrawList();
    for (int i = 0; i < components; i++) {
        ImGui::PushID(i);
        if (i > 0) {
            ImGui::SameLine(0, 0);
        }
        if (haveLabelButtons) {
            auto cursorPos = window->DC.CursorPos;
            if (i == 0) {
                float rounding = style.FrameRounding;
                DL->PathLineTo(
                    ImVec2(cursorPos.x + labelButtonSize.x, cursorPos.y));
                DL->PathLineTo(ImVec2(cursorPos.x + labelButtonSize.x,
                                      cursorPos.y + labelButtonSize.y));
                DL->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                         cursorPos.y + labelButtonSize.y -
                                             rounding),
                                  rounding, 3, 6);
                DL->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                         cursorPos.y + rounding),
                                  rounding, 6, 9);
                DL->PathFillConvex(colors[i]);
            }
            else {
                DL->AddRectFilled(cursorPos, cursorPos + labelButtonSize,
                                  colors[i]);
            }
            auto cp = ImGui::GetCursorPos();
            auto label = labels[i];
            withFont(FontDesc::UIDefault().setWeight(FontWeight::Black), [&] {
                auto textSize = ImGui::CalcTextSize(label);
                ImGui::SetCursorPos(
                    cp +
                    ImVec2((labelButtonSize.x - textSize.x) / 2,
                           i == 0 ? (labelButtonSize.y - textSize.y) / 2 : 0));
                ImGui::PushStyleColor(ImGuiCol_Text, 0xFFffFFff);
                ImGui::Text("%s", labels[i]);
                ImGui::PopStyleColor();
                ImGui::SetCursorPos(cp + ImVec2(labelButtonSize.x, 0));
            });
        }
        auto cursorPos = window->DC.CursorPos;
        ImGui::SetNextItemWidth(itemWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x0);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0x0);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0x0);
        ImGui::PushStyleColor(ImGuiCol_Border, 0x0);
        value_changed |=
            ImGui::DragFloat("", &v[i], v_speed, v_min, v_max, format, flags);
        ImGui::PopStyleColor(4);
        auto [hovered, active] =
            std::tuple{ ImGui::IsItemHovered(), ImGui::IsItemActive() };
        ImU32 col = ImGui::GetColorU32(active  ? ImGuiCol_FrameBgActive :
                                       hovered ? ImGuiCol_FrameBgHovered :
                                                 ImGuiCol_FrameBg);
        if (i == 0 && !haveLabelButtons) {
            float rounding = style.FrameRounding;
            DL->PathLineTo(ImVec2(cursorPos.x + itemWidth, cursorPos.y));
            DL->PathLineTo(ImVec2(cursorPos.x + itemWidth,
                                  cursorPos.y + labelButtonSize.y));
            DL->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                     cursorPos.y + height - rounding),
                              rounding, 3, 6);
            DL->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                     cursorPos.y + rounding),
                              rounding, 6, 9);
            DL->PathFillConvex(col);
        }
        else if (i < components - 1) {
            DL->AddRectFilled(cursorPos, cursorPos + ImVec2(itemWidth, height),
                              col);
        }
        else {
            float rounding = style.FrameRounding;
            DL->PathLineTo(cursorPos);
            DL->PathArcToFast(ImVec2(cursorPos.x + itemWidth - rounding,
                                     cursorPos.y + rounding),
                              rounding, 9, 12);
            DL->PathArcToFast(ImVec2(cursorPos.x + itemWidth - rounding,
                                     cursorPos.y + height - rounding),
                              rounding, 0, 3);
            DL->PathLineTo(ImVec2(cursorPos.x, cursorPos.y + height));
            DL->PathFillConvex(col);
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    /// Draw border
    if (style.FrameBorderSize > 0) {
        auto const col = ImGui::GetColorU32(ImGuiCol_Border);
        DL->AddRect(beginCursor, beginCursor + ImVec2(fullWidth, height), col,
                    style.FrameRounding);
        for (int i = 0; i < components; ++i) {
            /// Left
            if (i > 0) {
                DL->PathLineTo(beginCursor + ImVec2(i * (fullWidth / 3), 0));
                DL->PathLineTo(beginCursor +
                               ImVec2(i * (fullWidth / 3), height));
                DL->PathStroke(col);
            }
            if (!haveLabelButtons) {
                continue;
            }
            /// Right
            DL->PathLineTo(
                beginCursor +
                ImVec2(i * ImFloor(fullWidth / 3) + labelButtonSize.x, 0));
            DL->PathLineTo(
                beginCursor +
                ImVec2(i * ImFloor(fullWidth / 3) + labelButtonSize.x, height));
            DL->PathStroke(col);
        }
    }
    char const* label_end = ImGui::FindRenderedTextEnd(label);
    if (label != label_end) {
        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(label, label_end);
    }
    ImGui::EndGroup();
    return value_changed;
}

std::array<char, 64> poppy::generateUniqueID(std::string_view name, int id,
                                             bool prepentDoubleHash) {
    std::array<char, 64> buffer{};
    char* bufferPtr = buffer.data();
    /* -0000000 */
    std::size_t availSize = 56;
    if (prepentDoubleHash) {
        bufferPtr[0] = '#';
        bufferPtr[1] = '#';
        bufferPtr += 2;
        availSize -= 2;
    }
    std::size_t nameSize =
        std::min(name.size(), availSize - 2 * !prepentDoubleHash);
    std::memcpy(bufferPtr, name.data(), nameSize);
    bufferPtr += nameSize;
    availSize -= nameSize;
    if (!prepentDoubleHash) {
        bufferPtr[0] = '#';
        bufferPtr[1] = '#';
        bufferPtr += 2;
        availSize -= 2;
    }
    assert(availSize < 56 && "Overflow has occured");
    std::strncpy(bufferPtr, utl::strcat("-", id).data(), 8);
    return buffer;
}

static void displayEmptyImpl(std::string_view reason, FontDesc font,
                             float4 color) {
    float2 stashedCursorPos = ImGui::GetCursorPos();
    withFont(font, [&] {
        float2 textSize = ImGui::CalcTextSize(reason.data());
        ImGui::SetCursorPos(
            ((float2)ImGui::GetContentRegionAvail() - textSize) / 2);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s", reason.data());
        ImGui::PopStyleColor();
    });
    ImGui::SetCursorPos(stashedCursorPos);
}

static FontDesc emptyPlaceholderFont() {
    return FontDesc::UIDefault().setWeight(FontWeight::Semibold);
}

void poppy::displayEmptyWithReason(std::string_view reason) {
    displayEmptyImpl(reason, emptyPlaceholderFont(),
                     ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
}

void poppy::displayEmptyWithError(std::string_view reason) {
    displayEmptyImpl(reason, emptyPlaceholderFont(), { 1, 0, .2, 1 });
}
