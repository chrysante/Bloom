#include "Poppy/Core/SelectionManager.h"

#include <imgui.h>

using namespace poppy;

SelectionMode poppy::getSelectionMode() {
    if (ImGui::IsKeyDown(ImGuiMod_Super)) {
        return SelectionMode::Toggle;
    }
    if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
        return SelectionMode::Add;
    }
    return SelectionMode::Replace;
}
