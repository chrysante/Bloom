#include "Poppy/Core/SelectionManager.h"

#include <imgui.h>

using namespace poppy;

SelectionMode poppy::getSelectionMode() {
    if (ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
        return SelectionMode::Toggle;
    }
    if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
        return SelectionMode::Add;
    }
    return SelectionMode::Replace;
}
