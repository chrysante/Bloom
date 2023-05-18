#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/ImGuiHelpers.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <utl/strcat.hpp>

#include "Poppy/Core/Debug.hpp"

using namespace poppy;

static bool ViewportButtonBehavior(ImRect const& bb,
                                   ImGuiID id,
                                   bool* out_hovered,
                                   bool* out_held,
                                   ImGuiButtonFlags flags);

ViewportInput poppy::detectViewportInput(ImGuiButtonFlags buttonFlags) {
    using namespace ImGui;

    buttonFlags |= ImGuiButtonFlags_NoNavFocus;

    // save to restore later
    ImVec2 const cursorPos = GetCursorPos();
    SetCursorPos({ 0, 0 });

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return ViewportInput{};

    const ImGuiID id = window->GetID("__Invisible_Button__");

    ImVec2 size = CalcItemSize(window->Size, 0.0f, 0.0f);
    const ImRect bb(window->DC.CursorPos,
                    mtl::float2(window->DC.CursorPos) + mtl::float2(size));
    ItemSize(size);
    if (!ItemAdd(bb, id)) return {};

    ViewportInput result{};
    result.pressed = ViewportButtonBehavior(bb,
                                            id,
                                            &result.hovered,
                                            &result.held,
                                            buttonFlags);

    SetCursorPos(cursorPos);

    return result;
}

static bool ViewportButtonBehavior(ImRect const& bb,
                                   ImGuiID id,
                                   bool* out_hovered,
                                   bool* out_held,
                                   ImGuiButtonFlags flags) {
    using namespace ImGui;
    ::ImGuiContext& g   = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Default only reacts to left mouse button
    if ((flags & ImGuiButtonFlags_MouseButtonMask_) == 0)
        flags |= ImGuiButtonFlags_MouseButtonDefault_;

    // Default behavior requires click + release inside bounding box
    if ((flags & ImGuiButtonFlags_PressedOnMask_) == 0)
        flags |= ImGuiButtonFlags_PressedOnDefault_;

    ImGuiWindow* backup_hovered_window = g.HoveredWindow;
    bool const flatten_hovered_children =
        (flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredWindow &&
        g.HoveredWindow->RootWindowDockTree == window->RootWindowDockTree;
    if (flatten_hovered_children) g.HoveredWindow = window;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (id != 0 && g.LastItemData.ID != id) IMGUI_TEST_ENGINE_ITEM_ADD(bb, id);
#endif

    bool pressed = false;
    bool hovered =
        ItemHoverable(bb, /* id */ 0); // this needs to be zero to not interfere
                                       // with global hover stuff

    // Drag source doesn't report as hovered
    if (hovered && g.DragDropActive && g.DragDropPayload.SourceId == id &&
        !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoDisableHover))
        hovered = false;

    if (flatten_hovered_children) g.HoveredWindow = backup_hovered_window;

    // AllowOverlap mode (rarely used) requires previous frame HoveredId to be
    // null or to match. This allows using patterns where a later submitted
    // widget overlaps a previous one.
    if (hovered && (flags & ImGuiButtonFlags_AllowItemOverlap) &&
        (g.HoveredIdPreviousFrame != id && g.HoveredIdPreviousFrame != 0))
        hovered = false;

    // Mouse handling
    if (hovered) {
        if (!(flags & ImGuiButtonFlags_NoKeyModifiers) ||
            (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt))
        {
            // Poll buttons
            int mouse_button_clicked  = -1;
            int mouse_button_released = -1;
            if ((flags & ImGuiButtonFlags_MouseButtonLeft) &&
                g.IO.MouseClicked[0])
            {
                mouse_button_clicked = 0;
            }
            else if ((flags & ImGuiButtonFlags_MouseButtonRight) &&
                     g.IO.MouseClicked[1])
            {
                mouse_button_clicked = 1;
            }
            else if ((flags & ImGuiButtonFlags_MouseButtonMiddle) &&
                     g.IO.MouseClicked[2])
            {
                mouse_button_clicked = 2;
            }
            if ((flags & ImGuiButtonFlags_MouseButtonLeft) &&
                g.IO.MouseReleased[0])
            {
                mouse_button_released = 0;
            }
            else if ((flags & ImGuiButtonFlags_MouseButtonRight) &&
                     g.IO.MouseReleased[1])
            {
                mouse_button_released = 1;
            }
            else if ((flags & ImGuiButtonFlags_MouseButtonMiddle) &&
                     g.IO.MouseReleased[2])
            {
                mouse_button_released = 2;
            }

            if (mouse_button_clicked != -1 && g.ActiveId != id) {
                if (flags & (ImGuiButtonFlags_PressedOnClickRelease |
                             ImGuiButtonFlags_PressedOnClickReleaseAnywhere))
                {
                    SetActiveID(id, window);
                    g.ActiveIdMouseButton = mouse_button_clicked;
                    if (!(flags & ImGuiButtonFlags_NoNavFocus))
                        SetFocusID(id, window);
                    FocusWindow(window);
                }
                if ((flags & ImGuiButtonFlags_PressedOnClick) ||
                    ((flags & ImGuiButtonFlags_PressedOnDoubleClick) &&
                     g.IO.MouseClickedCount[mouse_button_clicked] == 2))
                {
                    pressed = true;
                    if (flags & ImGuiButtonFlags_NoHoldingActiveId)
                        ClearActiveID();
                    else SetActiveID(id, window); // Hold on ID
                    if (!(flags & ImGuiButtonFlags_NoNavFocus))
                        SetFocusID(id, window);
                    g.ActiveIdMouseButton = mouse_button_clicked;
                    FocusWindow(window);
                }
            }
            if ((flags & ImGuiButtonFlags_PressedOnRelease) &&
                mouse_button_released != -1)
            {
                // Repeat mode trumps on release behavior
                bool const has_repeated_at_least_once =
                    (flags & ImGuiButtonFlags_Repeat) &&
                    g.IO.MouseDownDurationPrev[mouse_button_released] >=
                        g.IO.KeyRepeatDelay;
                if (!has_repeated_at_least_once) pressed = true;
                if (!(flags & ImGuiButtonFlags_NoNavFocus))
                    SetFocusID(id, window);
                ClearActiveID();
            }

            // 'Repeat' mode acts when held regardless of _PressedOn flags (see
            // table above). Relies on repeat logic of IsMouseClicked() but we
            // may as well do it ourselves if we end up exposing finer
            // RepeatDelay/RepeatRate settings.
            if (g.ActiveId == id && (flags & ImGuiButtonFlags_Repeat))
                if (g.IO.MouseDownDuration[g.ActiveIdMouseButton] > 0.0f &&
                    IsMouseClicked(g.ActiveIdMouseButton, true))
                    pressed = true;
        }

        if (pressed) g.NavDisableHighlight = true;
    }

    // Gamepad/Keyboard navigation
    // We report navigated item as hovered but we don't set g.HoveredId to not
    // interfere with mouse.
    if (g.NavId == id && !g.NavDisableHighlight && g.NavDisableMouseHover &&
        (g.ActiveId == 0 || g.ActiveId == id || g.ActiveId == window->MoveId))
        if (!(flags & ImGuiButtonFlags_NoHoveredOnFocus)) hovered = true;
    if (g.NavActivateDownId == id) {
        bool nav_activated_by_code = (g.NavActivateId == id);
        bool nav_activated_by_inputs =
            IsNavInputTest(ImGuiNavInput_Activate,
                           (flags & ImGuiButtonFlags_Repeat) ?
                               ImGuiInputReadMode_Repeat :
                               ImGuiInputReadMode_Pressed);
        if (nav_activated_by_code || nav_activated_by_inputs) {
            // Set active id so it can be queried by user via IsItemActive(),
            // equivalent of holding the mouse button.
            pressed = true;
            SetActiveID(id, window);
            g.ActiveIdSource = ImGuiInputSource_Nav;
            if (!(flags & ImGuiButtonFlags_NoNavFocus)) SetFocusID(id, window);
        }
    }

    // Process while held
    bool held = false;
    if (g.ActiveId == id) {
        if (g.ActiveIdSource == ImGuiInputSource_Mouse) {
            if (g.ActiveIdIsJustActivated)
                g.ActiveIdClickOffset = g.IO.MousePos - bb.Min;

            int const mouse_button = g.ActiveIdMouseButton;
            IM_ASSERT(mouse_button >= 0 &&
                      mouse_button < ImGuiMouseButton_COUNT);
            if (g.IO.MouseDown[mouse_button]) {
                held = true;
            }
            else {
                bool release_in =
                    hovered &&
                    (flags & ImGuiButtonFlags_PressedOnClickRelease) != 0;
                bool release_anywhere =
                    (flags & ImGuiButtonFlags_PressedOnClickReleaseAnywhere) !=
                    0;
                if ((release_in || release_anywhere) && !g.DragDropActive) {
                    // Report as pressed when releasing the mouse (this is the
                    // most common path)
                    bool is_double_click_release =
                        (flags & ImGuiButtonFlags_PressedOnDoubleClick) &&
                        g.IO.MouseReleased[mouse_button] &&
                        g.IO.MouseClickedLastCount[mouse_button] == 2;
                    bool is_repeating_already =
                        (flags & ImGuiButtonFlags_Repeat) &&
                        g.IO.MouseDownDurationPrev[mouse_button] >=
                            g.IO.KeyRepeatDelay; // Repeat mode trumps <on
                                                 // release>
                    if (!is_double_click_release && !is_repeating_already)
                        pressed = true;
                }
                ClearActiveID();
            }
            if (!(flags & ImGuiButtonFlags_NoNavFocus))
                g.NavDisableHighlight = true;
        }
        else if (g.ActiveIdSource == ImGuiInputSource_Nav) {
            // When activated using Nav, we hold on the ActiveID until
            // activation button is released
            if (g.NavActivateDownId != id) ClearActiveID();
        }
        if (pressed) g.ActiveIdHasBeenPressedBefore = true;
    }

    if (out_hovered) *out_hovered = hovered;
    if (out_held) *out_held = held;

    return pressed;
}

bool poppy::dragFloat3Pretty(char const* label,
                             float v[3],
                             float v_speed,
                             float v_min,
                             float v_max,
                             char const* format,
                             ImGuiSliderFlags flags) {
    int const components = 3;
    using namespace ImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    auto& style     = GetStyle();

    bool value_changed = false;

    float const height =
        CalcTextSize("", NULL, true).y + style.FramePadding.y * 2.0f;

    BeginGroup();
    PushID(label);

    constexpr std::array colors = { ImU32(0xFF1111FFu),
                                    ImU32(0xFF00CC00u),
                                    ImU32(0xFFFF2222u) };
    constexpr std::array labels = { "X", "Y", "Z" };

    ImVec2 labelButtonSize      = { height * 1.5f, height };
    float const fullWidth       = CalcItemWidth();
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
                auto* const dl       = window->DrawList;
                float const rounding = style.FrameRounding;
                dl->PathLineTo(
                    ImVec2(cursorPos.x + labelButtonSize.x, cursorPos.y));
                dl->PathLineTo(ImVec2(cursorPos.x + labelButtonSize.x,
                                      cursorPos.y + labelButtonSize.y));
                dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                         cursorPos.y + labelButtonSize.y -
                                             rounding),
                                  rounding,
                                  3,
                                  6);
                dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                         cursorPos.y + rounding),
                                  rounding,
                                  6,
                                  9);
                dl->PathFillConvex(colors[i]);
            }
            else {
                window->DrawList->AddRectFilled(cursorPos,
                                                cursorPos + labelButtonSize,
                                                colors[i]);
            }

            auto const cp    = GetCursorPos();
            auto const label = labels[i];
            //				withFont(Font::UIDefault().setWeight(FontWeight::black),
            //[&]{ 					auto const textSize = CalcTextSize(label);
            // SetCursorPos(cp + ImVec2((labelButtonSize.x - textSize.x) / 2, i
            // == 0 ? (labelButtonSize.y
            //- textSize.y) / 2 : 0));
            // ImGui::PushStyleColor(ImGuiCol_Text,
            // 0xff000000); 					Text("%s", labels[i]);
            // ImGui::PopStyleColor(); 					SetCursorPos(cp +
            // ImVec2(labelButtonSize.x, 0));
            //				});

            withFont(Font::UIDefault().setWeight(FontWeight::black), [&] {
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
            auto* const dl       = window->DrawList;
            float const rounding = style.FrameRounding;
            dl->PathLineTo(ImVec2(cursorPos.x + itemWidth, cursorPos.y));
            dl->PathLineTo(ImVec2(cursorPos.x + itemWidth,
                                  cursorPos.y + labelButtonSize.y));
            dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                     cursorPos.y + height - rounding),
                              rounding,
                              3,
                              6);
            dl->PathArcToFast(ImVec2(cursorPos.x + rounding,
                                     cursorPos.y + rounding),
                              rounding,
                              6,
                              9);
            dl->PathFillConvex(col);
        }
        else if (i < components - 1) {
            window->DrawList->AddRectFilled(cursorPos,
                                            cursorPos +
                                                ImVec2(itemWidth, height),
                                            col);
        }
        else {
            auto* const dl       = window->DrawList;
            float const rounding = style.FrameRounding;
            dl->PathLineTo(cursorPos);
            dl->PathArcToFast(ImVec2(cursorPos.x + itemWidth - rounding,
                                     cursorPos.y + rounding),
                              rounding,
                              9,
                              12);
            dl->PathArcToFast(ImVec2(cursorPos.x + itemWidth - rounding,
                                     cursorPos.y + height - rounding),
                              rounding,
                              0,
                              3);
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
                                  beginCursor + ImVec2(fullWidth, height),
                                  col,
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

std::array<char, 64> poppy::generateUniqueID(std::string_view name,
                                             int id,
                                             bool const prepentDoubleHash) {
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

    std::size_t const nameSize =
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
