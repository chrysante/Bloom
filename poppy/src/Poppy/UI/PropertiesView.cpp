#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/PropertiesView.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <mtl/mtl.hpp>

#include "Poppy/Core/Debug.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace mtl::short_types;
using namespace poppy;
using namespace propertiesView;

void propertiesView::header(std::string_view name, FontDesc const& font) {
    withFont(font, [&] {
        float4 color = GImGui->Style.Colors[ImGuiCol_Text];
        color.a *= 0.85;
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s", name.data());
        ImGui::PopStyleColor();
    });
}

bool propertiesView::beginSection() {
    bool const open =
        ImGui::BeginTable("Property Table", 2,
                          ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_NoBordersInBodyUntilResize);
    if (open) {
        ImGui::TableSetupColumn(
            "Label",
            ImGuiTableColumnFlags_WidthStretch); // Default to 100.0f
        ImGui::TableSetupColumn(
            "Property",
            ImGuiTableColumnFlags_WidthStretch); // Default to 200.0f
    }
    return open;
}

bool propertiesView::beginSection(std::string_view name, FontDesc const& font) {
    header(name, font);
    return beginSection();
}

void propertiesView::endSection() { ImGui::EndTable(); }

void propertiesView::beginProperty(std::string_view label) {
    beginProperty(label, FontDesc::UIDefault().setWeight(FontWeight::light));
}

void propertiesView::beginProperty(std::string_view label,
                                   FontDesc const& font) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    withFont(font, [&] {
        auto& style = ImGui::GetStyle();
        auto const textSize = ImGui::CalcTextSize(label.data());
        auto const availWidth = ImGui::GetContentRegionAvail().x;
        float const cpY = ImGui::GetCursorPosY();
        float const cpX =
            std::max(style.WindowPadding.x, availWidth - textSize.x);

        ImGui::SetCursorPosX(cpX);
        ImGui::Text("%s", label.data());
        if (cpX == style.WindowPadding.x && ImGui::IsItemHovered()) {
            ImGui::SetNextWindowPos(ImGui::GetWindowPos() + ImVec2(cpX, cpY) -
                                    style.WindowPadding);
            ImGui::SetTooltip("%s", label.data());
        }
    });

    ImGui::TableSetColumnIndex(1);
}

void propertiesView::fullWidth(float offset) {
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x + offset);
}
