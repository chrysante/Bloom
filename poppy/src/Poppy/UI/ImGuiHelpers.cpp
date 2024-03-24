#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <mtl/mtl.hpp>
#include <utl/strcat.hpp>

#include "Poppy/Core/Debug.h"

using namespace poppy;

ViewportInput poppy::detectViewportInput(ImGuiButtonFlags buttonFlags) {
    using namespace ImGui;
    buttonFlags |= ImGuiButtonFlags_NoNavFocus;
    // Save to restore later
    ImVec2 cursorPos = GetCursorPos();
    ImGuiID hovered = ImGui::GetCurrentContext()->HoveredId;
    SetCursorPos({ 0, 0 });
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return ViewportInput{};
    ImGuiID id = window->GetID("__Invisible_Button__");
    ImVec2 size = CalcItemSize(window->Size, 0.0f, 0.0f);
    ImRect bb(window->DC.CursorPos,
              mtl::float2(window->DC.CursorPos) + mtl::float2(size));
    ItemSize(size);
    if (!ItemAdd(bb, id)) return {};
    ViewportInput result{};
    result.pressed =
        ButtonBehavior(bb, id, &result.hovered, &result.held, buttonFlags);
    ImGui::SetHoveredID(hovered);
    SetCursorPos(cursorPos);
    return result;
}

bool poppy::dragFloat3Pretty(char const* label, float v[3], float v_speed,
                             float v_min, float v_max, char const* format,
                             ImGuiSliderFlags flags) {
    int const components = 3;
    using namespace ImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    auto& style = GetStyle();

    bool value_changed = false;

    float const height =
        CalcTextSize("", NULL, true).y + style.FramePadding.y * 2.0f;

    BeginGroup();
    PushID(label);

    constexpr std::array colors = { ImU32(0xFF1111FFu), ImU32(0xFF00CC00u),
                                    ImU32(0xFFFF2222u) };
    constexpr std::array labels = { "X", "Y", "Z" };

    ImVec2 labelButtonSize = { height * 1.5f, height };
    float const fullWidth = CalcItemWidth();
    bool const haveLabelButtons = fullWidth / components >= 90;
    float const itemWidth =
        fullWidth / components - (haveLabelButtons ? labelButtonSize.x : 0);
    auto const beginCursor = window->DC.CursorPos;
    for (int i = 0; i < components; i++) {
        PushID(i);
        if (i > 0) SameLine(0, 0);

        if (haveLabelButtons) {

            auto const cursorPos = window->DC.CursorPos;
            if (i == 0) {
                auto* const dl = window->DrawList;
                float const rounding = style.FrameRounding;
                dl->PathLineTo(
                    ImVec2(cursorPos.x + labelButtonSize.x, cursorPos.y));
                dl->PathLineTo(ImVec2(cursorPos.x + labelButtonSize.x,
                                      cursorPos.y + labelButtonSize.y));
                dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                         cursorPos.y + labelButtonSize.y -
                                             rounding),
                                  rounding, 3, 6);
                dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                         cursorPos.y + rounding),
                                  rounding, 6, 9);
                dl->PathFillConvex(colors[i]);
            }
            else {
                window->DrawList->AddRectFilled(cursorPos,
                                                cursorPos + labelButtonSize,
                                                colors[i]);
            }

            auto const cp = GetCursorPos();
            auto const label = labels[i];
            //				withFont(FontDesc::UIDefault().setWeight(FontWeight::black),
            //[&]{ 					auto const textSize = CalcTextSize(label);
            // SetCursorPos(cp + ImVec2((labelButtonSize.x - textSize.x) / 2, i
            // == 0 ? (labelButtonSize.y
            //- textSize.y) / 2 : 0));
            // ImGui::PushStyleColor(ImGuiCol_Text,
            // 0xff000000); 					Text("%s", labels[i]);
            // ImGui::PopStyleColor(); 					SetCursorPos(cp +
            // ImVec2(labelButtonSize.x, 0));
            //				});

            withFont(FontDesc::UIDefault().setWeight(FontWeight::black), [&] {
                //					auto const label = labels[i];
                auto const textSize = CalcTextSize(label);
                //					auto const cp = GetCursorPos();
                SetCursorPos(
                    cp +
                    ImVec2((labelButtonSize.x - textSize.x) / 2,
                           i == 0 ? (labelButtonSize.y - textSize.y) / 2 : 0));
                ImGui::PushStyleColor(ImGuiCol_Text, 0xFFffFFff);
                Text("%s", labels[i]);
                ImGui::PopStyleColor();
                SetCursorPos(cp + ImVec2(labelButtonSize.x, 0));
            });
        }

        auto const cursorPos = window->DC.CursorPos;

        ImGui::SetNextItemWidth(itemWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x0);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0x0);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0x0);
        ImGui::PushStyleColor(ImGuiCol_Border, 0x0);
        value_changed |=
            DragFloat("", &v[i], v_speed, v_min, v_max, format, flags);
        ImGui::PopStyleColor(4);

        auto const [hovered, active] =
            std::tuple{ ImGui::IsItemHovered(), ImGui::IsItemActive() };
        ImU32 const col = GetColorU32(active  ? ImGuiCol_FrameBgActive :
                                      hovered ? ImGuiCol_FrameBgHovered :
                                                ImGuiCol_FrameBg);
        if (i == 0 && !haveLabelButtons) {
            auto* const dl = window->DrawList;
            float const rounding = style.FrameRounding;
            dl->PathLineTo(ImVec2(cursorPos.x + itemWidth, cursorPos.y));
            dl->PathLineTo(ImVec2(cursorPos.x + itemWidth,
                                  cursorPos.y + labelButtonSize.y));
            dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                     cursorPos.y + height - rounding),
                              rounding, 3, 6);
            dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                     cursorPos.y + rounding),
                              rounding, 6, 9);
            dl->PathFillConvex(col);
        }
        else if (i < components - 1) {
            window->DrawList->AddRectFilled(cursorPos,
                                            cursorPos +
                                                ImVec2(itemWidth, height),
                                            col);
        }
        else {
            auto* const dl = window->DrawList;
            float const rounding = style.FrameRounding;
            dl->PathLineTo(cursorPos);
            dl->PathArcToFast(ImVec2(cursorPos.x + itemWidth - rounding,
                                     cursorPos.y + rounding),
                              rounding, 9, 12);
            dl->PathArcToFast(ImVec2(cursorPos.x + itemWidth - rounding,
                                     cursorPos.y + height - rounding),
                              rounding, 0, 3);
            dl->PathLineTo(ImVec2(cursorPos.x, cursorPos.y + height));
            dl->PathFillConvex(col);
        }

        PopID();
    }
    PopID();

    // draw border
    if (style.FrameBorderSize > 0) {
        auto const col = GetColorU32(ImGuiCol_Border);
        window->DrawList->AddRect(beginCursor,
                                  beginCursor + ImVec2(fullWidth, height), col,
                                  style.FrameRounding);

        for (int i = 0; i < components; ++i) {
            // left
            if (i > 0) {
                window->DrawList->PathLineTo(beginCursor +
                                             ImVec2(i * (fullWidth / 3), 0));
                window->DrawList->PathLineTo(
                    beginCursor + ImVec2(i * (fullWidth / 3), height));
                window->DrawList->PathStroke(col);
            }
            if (!haveLabelButtons) {
                continue;
            }
            // right
            window->DrawList->PathLineTo(
                beginCursor +
                ImVec2(i * ImFloor(fullWidth / 3) + labelButtonSize.x, 0));
            window->DrawList->PathLineTo(
                beginCursor +
                ImVec2(i * ImFloor(fullWidth / 3) + labelButtonSize.x, height));
            window->DrawList->PathStroke(col);
        }
    }

    char const* label_end = FindRenderedTextEnd(label);
    if (label != label_end) {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
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
