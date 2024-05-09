#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/view.hpp>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Poppy/Core/Debug.h"

using namespace poppy;
using namespace ranges::views;
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
    bool valueChanged = false;
    float height = ImGui::GetFontSize() + 2 * style.FramePadding.y;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    constexpr std::array colors = { IM_COL32(255, 17, 17, 255),
                                    IM_COL32(0, 204, 0, 255),
                                    IM_COL32(34, 127, 255, 255) };
    constexpr std::array labels = { "X", "Y", "Z" };
    constexpr std::array itemLabels = { "###X", "###Y", "###Z" };
    /// We display labels if each component is at least 90 pixels in width
    float fullWidth = 3 * ImFloor(ImGui::CalcItemWidth() / 3);
    bool haveLabelButtons = fullWidth / components >= 90;
    float2 labelButtonSize = float2(haveLabelButtons ? height : 0.0f);
    float itemWidth = fullWidth / components - labelButtonSize.x;
    float2 beginCursor = window->DC.CursorPos;
    auto* DL = ImGui::GetWindowDrawList();
    float rounding = style.FrameRounding;
    float2 min = beginCursor;
    float2 max = beginCursor + float2(fullWidth, height);
    DL->AddRect(min + float2(1, 1), max + float2(1, 1),
                ImGui::GetColorU32(ImGuiCol_BorderShadow), rounding, 0, 1);
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    for (int i = 0; i < components; i++) {
        float2 cursorPos =
            beginCursor +
            float2(i * (itemWidth + labelButtonSize.x) + labelButtonSize.x, 0);
        ImGui::SetCursorScreenPos(cursorPos);
        ImGui::SetNextItemWidth(itemWidth);
        ImDrawFlags drawFlags = i == 0 ? (haveLabelButtons ?
                                              ImDrawFlags_RoundCornersNone :
                                              ImDrawFlags_RoundCornersLeft) :
                                i == 1 ? ImDrawFlags_RoundCornersNone :
                                         ImDrawFlags_RoundCornersRight;
        valueChanged |= ImGui::DragFloat(itemLabels[i], &v[i], v_speed, v_min,
                                         v_max, format, flags, drawFlags);
    }
    ImGui::PopStyleColor(2);
    if (haveLabelButtons) {
        for (int i = 0; i < components; i++) {
            ImDrawFlags drawFlags = i == 0 ? ImDrawFlags_RoundCornersLeft :
                                             ImDrawFlags_RoundCornersNone;
            float2 cursorPos = beginCursor + float2(i * fullWidth / 3, 0);
            float2 pMin = cursorPos - float2(1, 0);
            float2 pMax = cursorPos + labelButtonSize + float2(1, 0);
            DL->AddRectFilled(pMin, pMax, colors[i], rounding, drawFlags);
            char const* label = labels[i];
            withFont(FontDesc::UIDefault().setWeight(FontWeight::Black), [&] {
                auto textSize = ImGui::CalcTextSize(label);
                float2 pos = cursorPos + (labelButtonSize - textSize) / 2;
                DL->AddText(pos, 0xFFffFFff, labels[i]);
            });
        }
    }
    DL->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0,
                style.FrameBorderSize);
    ImGui::PopID(); /// Label
    char const* labelEnd = ImGui::FindRenderedTextEnd(label);
    if (label != labelEnd) {
        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(label, labelEnd);
    }
    ImGui::EndGroup();
    return valueChanged;
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

void poppy::splitView(
    std::string_view strID,
    std::initializer_list<utl::function_view<void()>> contents) {
    ImGuiID ID = ImGui::GetID(strID.data(), strID.data() + strID.size());
    static float totalWidth = ImGui::GetContentRegionAvail().x;
    static std::vector<float> widths;
    if (widths.size() != contents.size()) {
        widths.resize(contents.size(), totalWidth / contents.size());
    }
    float newWidth = ImGui::GetContentRegionAvail().x;
    if (totalWidth != newWidth) {
        float factor = newWidth / totalWidth;
        for (auto& width: widths) {
            width *= factor;
        }
        totalWidth = newWidth;
    }
    ImGui::PushID(ID);
    float2 cursorPos = ImGui::GetCursorScreenPos();
    auto* window = ImGui::GetCurrentWindow();
    float acc = 0;
    float height = ImGui::GetContentRegionAvail().y;
    for (auto [index, width, content]: zip(iota(0), widths, contents)) {
        float widthCopy = width;
        float2 pos = cursorPos + float2(acc, 0);
        float halfSplitterWidth = 1;
        if (index > 0) {
            ImGui::SplitterBehavior(ImRect(pos - float2(halfSplitterWidth, 0),
                                           pos + float2(halfSplitterWidth,
                                                        height)),
                                    window->GetID(-index), ImGuiAxis_X,
                                    &widths[index - 1], &widths[index],
                                    /* minsize1: */ 10,
                                    /* minsize2: */ 10,
                                    /* hover_extend: */ 4.0f,
                                    /* hover_visibility_delay */ 0.04f);
            pos.x += halfSplitterWidth;
        }
        if (index < (int)contents.size() - 1) {
            widthCopy -= 2 * halfSplitterWidth;
        }
        ImGui::SetNextWindowPos(pos);
        if (ImGui::BeginChild(window->GetID(index), { widthCopy, 0 })) {
            content();
        }
        ImGui::EndChild();
        acc += width;
    }
    ImGui::PopID();
}
