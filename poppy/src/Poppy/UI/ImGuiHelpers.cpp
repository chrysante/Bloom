#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/numeric.hpp>
#include <range/v3/view.hpp>
#include <utl/functional.hpp>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/Application.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Messages.h"
#include "Poppy/UI/Appearance.h"

using namespace poppy;
using namespace bloom;
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

std::array<float, 3> computeItemWidths(int fullWidth, float labelSize) {
    int r = fullWidth % 3;
    int itemWidth = fullWidth / 3 - labelSize;
    return {
        float(itemWidth),
        float(itemWidth) + (r > 0),
        float(itemWidth) + (r > 1),
    };
}

static float defaultItemHeight() {
    return ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2;
}

bool poppy::linkButton(bool& linked, float2 size, IconSize iconSize) {
    if (size.x <= 0) {
        size.x = defaultItemHeight();
    }
    if (size.y <= 0) {
        size.y = defaultItemHeight();
    }
    auto [iconFont, unicode] =
        FontManager::get({ iconSize }, linked ? "link" : "unlink");
    ImGui::PushFont(iconFont);
    bool linkedBackup = linked;
    if (linkedBackup) {
        auto& colors = Appearance::get().style().colors;
        ImGui::PushStyleColor(ImGuiCol_Button, colors.highlightControlFrame);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              colors.highlightControlFrameHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              colors.highlightControlFrameDown);
    }
    bool pressed = ImGui::Button(unicode.data(), size);
    if (pressed) {
        linked ^= true;
    }
    if (linkedBackup) {
        ImGui::PopStyleColor(3);
    }
    ImGui::PopFont();
    return pressed;
}

bool poppy::dragFloat3Labelled(char const* label, float v[3],
                               DragFloat3Options options) {
    float3 valueBackup([&](int i) { return v[i]; });
    int const components = 3;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    ImGuiContext& g = *GImGui;
    auto& style = ImGui::GetStyle();
    bool3 valueChanged = false;
    float height = ImGui::GetFontSize() + 2 * style.FramePadding.y;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    bool* linked = [&] {
        if (options.linkFields) {
            return &options.linkFields.value();
        }
        return window->StateStorage.GetBoolRef(ImGui::GetID("IsLinked"), false);
    }();
    float2 linkButtonSize =
        options.showLinkButton ? float2(defaultItemHeight()) : float2();
    constexpr std::array colors = { IM_COL32(255, 17, 17, 255),
                                    IM_COL32(0, 204, 0, 255),
                                    IM_COL32(34, 127, 255, 255) };
    constexpr std::array labels = { "X", "Y", "Z" };
    constexpr std::array itemLabels = { "###X", "###Y", "###Z" };
    float spacing = options.showLinkButton ? style.ItemSpacing.x : 0.0f;
    /// We display labels if each component is at least 90 pixels in width
    float fullWidth = ImGui::CalcItemWidth() - spacing - linkButtonSize.x;
    bool haveButtonLabels = [&] {
        using enum DragFloat3Options::LabelStyle;
        if (options.labelStyle == Auto) {
            return fullWidth / components >= 90;
        }
        return options.labelStyle == Wide;
    }();
    float2 labelButtonSize = haveButtonLabels ? float2(height, height) :
                                                float2(3.0f, height);
    std::array itemWidths = computeItemWidths(fullWidth, labelButtonSize.x);
    float2 beginCursor = window->DC.CursorPos;
    auto* DL = ImGui::GetWindowDrawList();
    float rounding = style.FrameRounding;
    float2 min = beginCursor;
    float2 max = beginCursor + float2(fullWidth, height);
    float2 linkButtonPos = min + float2(fullWidth + spacing, 0);
    DL->AddRect(min + float2(1, 1), max + float2(1, 1),
                ImGui::GetColorU32(ImGuiCol_BorderShadow), rounding, 0, 1);
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    float xCursor = 0;
    for (int i = 0; i < components; i++) {
        float2 cursorPos = beginCursor + float2(xCursor + labelButtonSize.x, 0);
        xCursor += itemWidths[i] + labelButtonSize.x;
        ImGui::SetCursorScreenPos(cursorPos);
        ImGui::SetNextItemWidth(itemWidths[i]);
        ImDrawFlags drawFlags = i == 2 ? ImDrawFlags_RoundCornersRight :
                                         ImDrawFlags_RoundCornersNone;
        valueChanged[i] |= ImGui::DragFloat(itemLabels[i], &v[i], options.speed,
                                            options.min, options.max,
                                            options.format, options.flags,
                                            drawFlags);
    }
    ImGui::PopStyleColor(2);
    for (int i = 0; i < components; i++) {
        ImDrawFlags drawFlags = i == 0 ? ImDrawFlags_RoundCornersLeft :
                                         ImDrawFlags_RoundCornersNone;
        float2 cursorPos = beginCursor + float2(i * fullWidth / 3, 0);
        float2 pMin = cursorPos - float2(1, 0);
        float2 pMax = cursorPos + labelButtonSize + float2(1, 0);
        DL->AddRectFilled(pMin, pMax, colors[i], rounding, drawFlags);
        if (haveButtonLabels) {
            char const* label = labels[i];
            withFont(FontDesc::UIDefault().setWeight(FontWeight::Black), [&] {
                auto textSize = ImGui::CalcTextSize(label);
                float2 pos = cursorPos + (labelButtonSize - textSize) / 2;
                DL->AddText(pos, IM_COL32_WHITE, labels[i]);
            });
        }
    }
    DL->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0,
                style.FrameBorderSize);
    if (options.showLinkButton) {
        ImGui::SetCursorScreenPos(linkButtonPos);
        linkButton(*linked, linkButtonSize);
    }
    ImGui::PopID();
    /// Label
    char const* labelEnd = ImGui::FindRenderedTextEnd(label);
    if (label != labelEnd) {
        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(label, labelEnd);
    }
    ImGui::EndGroup();
    bool anyChanged = valueChanged.fold(utl::logical_or);
    if (*linked && anyChanged) {
        for (size_t i = 0; i < 3; ++i) {
            if (valueChanged[i] && valueBackup[i] != 0.0) {
                auto ratio = v[i] / valueBackup[i];
                v[(i + 1) % 3] *= ratio;
                v[(i + 2) % 3] *= ratio;
            }
        }
    }
    return anyChanged;
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

static void displayEmptyImpl(std::string_view reason, float2 size,
                             FontDesc font, float4 color) {
    float2 stashedCursorPos = ImGui::GetCursorPos();
    size = float2([&](size_t index) {
        return size[index] <= 0 ? ImGui::GetContentRegionAvail()[index] :
                                  size[index];
    });
    withFont(font, [&] {
        float2 textSize = ImGui::CalcTextSize(reason.data());
        ImGui::SetCursorPos((size - textSize) / 2);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextEx(reason.data(), reason.data() + reason.size());
        ImGui::PopStyleColor();
    });
    ImGui::SetCursorPos(stashedCursorPos);
}

static FontDesc emptyPlaceholderFont() {
    return FontDesc::UIDefault().setWeight(FontWeight::Semibold);
}

void poppy::displayEmptyWithReason(std::string_view reason, float2 size) {
    displayEmptyImpl(reason, size, emptyPlaceholderFont(),
                     ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
}

void poppy::displayEmptyWithError(std::string_view reason, float2 size) {
    displayEmptyImpl(reason, size, emptyPlaceholderFont(), { 1, 0, .2, 1 });
}

void SplitView::display(std::span<std::function<void()> const> contents,
                        Flags flags) {
    ImGuiID imguiID = ImGui::GetID(ID.data(), ID.data() + ID.size());
    float2 contentSize = ImGui::GetContentRegionAvail();
    float newWidth = contentSize.x;
    if (widths.size() != contents.size()) {
        widths.resize(contents.size());
    }
    ranges::for_each(widths, [&](float& width) {
        if (width <= 0) {
            width = contentSize.x / contents.size();
        }
    });
    float sum = ranges::accumulate(widths, 0.0f);
    ranges::for_each(widths,
                     [&](float& width) { width = width * newWidth / sum; });
    if (totalWidth > 0 && totalWidth != newWidth) {
        float factor = newWidth / totalWidth;
        for (auto& width: widths) {
            width *= factor;
        }
        totalWidth = newWidth;
    }
    ImGui::PushID(imguiID);
    float2 cursorPos = ImGui::GetCursorScreenPos();
    auto* window = ImGui::GetCurrentWindow();
    float acc = 0;
    float height = ImGui::GetContentRegionAvail().y;
    float2 padding = ImGui::GetStyle().WindowPadding;
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
        ImRect windowBB(pos, pos + float2(widthCopy, contentSize.y));
        if ((flags & NoPadding) == 0) {
            windowBB.Expand(-padding);
        }
        ImGui::SetNextWindowPos(windowBB.Min);
        if (ImGui::BeginChild(window->GetID(index), windowBB.GetSize())) {
            content();
        }
        ImGui::EndChild();
        acc += width;
    }
    ImGui::PopID();
}

void SplitView::display(std::initializer_list<std::function<void()>> contents,
                        Flags flags) {
    display(std::span(contents), flags);
}

void poppy::markObjectRenderHazard(std::any obj) {
    Application::DispatchMessage(DispatchToken::Now,
                                 ExtendRenderLifetime{ std::move(obj) });
}
