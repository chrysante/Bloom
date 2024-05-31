#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/UI/PropertiesView.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vml/vml.hpp>

#include "Poppy/Core/Debug.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace vml::short_types;
using namespace poppy;
using namespace propertiesView;

void propertiesView::header(std::string_view name, FontDesc const& font) {
    withFont(font, [&] {
        float4 color = GImGui->Style.Colors[ImGuiCol_Text];
        color.a *= 0.85;
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextEx(name.data());
        ImGui::PopStyleColor();
    });
}

bool propertiesView::beginSection() {
    auto tableFlags = ImGuiTableFlags_Resizable |
                      ImGuiTableFlags_NoBordersInBodyUntilResize;
    bool open = ImGui::BeginTable("Property Table", 2, tableFlags);
    if (!open) {
        return false;
    }
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.3);
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);
    return true;
}

bool propertiesView::beginSection(std::string_view name, FontDesc const& font) {
    header(name, font);
    return beginSection();
}

void propertiesView::endSection() { ImGui::EndTable(); }

void propertiesView::beginProperty(std::string_view label) {
    beginProperty(label, FontDesc::UIDefault().setWeight(FontWeight::Light));
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
        auto* textBegin = label.data();
        auto* textEnd = label.data() + label.size();
        ImGui::TextEx(textBegin, textEnd);
        if (cpX == style.WindowPadding.x && ImGui::IsItemHovered()) {
            ImGui::SetNextWindowPos(ImGui::GetWindowPos() + ImVec2(cpX, cpY) -
                                    style.WindowPadding);
            if (ImGui::BeginTooltip()) {
                ImGui::TextEx(textBegin, textEnd);
                ImGui::EndTooltip();
            }
        }
    });

    ImGui::TableSetColumnIndex(1);
}

void propertiesView::fullWidth(float offset) {
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x + offset);
}
