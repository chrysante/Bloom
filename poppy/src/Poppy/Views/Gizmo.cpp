#include "Poppy/Views/Gizmo.h"

#include <array>
#include <optional>

#include <ImGuizmo/ImGuizmo.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <vml/vml.hpp>

#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/EntitySelectionManager.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

std::string poppy::toString(GizmoOperation op) {
    return std::array<std::string, 3>{ "Translate", "Rotate",
                                       "Scale" }[(size_t)op];
}
std::string poppy::toString(GizmoSpace sp) {
    return std::array<std::string, 2>{ "World", "Local" }[(size_t)sp];
}

Gizmo::Gizmo(): context(ImGuizmo::CreateContext()) {}

void Gizmo::setOperation(GizmoOperation op) {
    if (isUsing()) {
        /// Guard here because ImGuizmo crashes when changing the
        /// operation while using the gizmo.
        return;
    }
    _operation = op;
}

void Gizmo::setSpace(GizmoSpace space) {
    if (operation() == GizmoOperation::Scale) {
        space = GizmoSpace::Local;
    }
    _space[_operation] = space;
}

void Gizmo::cycleSpace() {
    auto newSpace = space() == GizmoSpace::World ? GizmoSpace::Local :
                                                   GizmoSpace::World;
    setSpace(newSpace);
}

static ImGuizmo::OPERATION translateOperation(GizmoOperation op) {
    return (ImGuizmo::OPERATION)op;
}

static ImGuizmo::MODE translateSpace(GizmoSpace space) {
    return (ImGuizmo::MODE)(1 - (int)space);
}

static std::optional<float4x4> manipulateGizmo(
    int ID, ImGuizmo::Context* context, vml::rectangle<float> rect,
    float4x4 view, float4x4 proj, GizmoOperation operation, GizmoSpace space,
    float4x4 transform) {
    ImGuizmo::SetContext(context);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    float2 pos = rect.lower_bound();
    float2 size = rect.size();
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
    view = vml::transpose(view);
    proj = vml::transpose(proj);
    transform = vml::transpose(transform);
    ImGuizmo::SetID(ID);
    if (ImGuizmo::Manipulate(view.data(), proj.data(),
                             translateOperation(operation),
                             translateSpace(space), transform.data()))
    {
        return vml::transpose(transform);
    }
    return std::nullopt;
}

bool Gizmo::display(Camera const& camera, vml::rectangle<float> rect,
                    vml::float4x4& wsTransform) {
    auto view = camera.view();
    auto proj = camera.projection();
    auto ID = ImGui::GetID("Gizmo");
    auto newEntityWSTransform =
        manipulateGizmo(ID, static_cast<ImGuizmo::Context*>(context.get()),
                        rect, view, proj, operation(), space(), wsTransform);
    _hovered = ImGuizmo::IsOver();
    bool usedLastFrame = _using;
    _using = newEntityWSTransform.has_value() ||
             (usedLastFrame && ImGui::IsMouseDown(ImGuiMouseButton_Left));
    if (!newEntityWSTransform.has_value()) {
        return false;
    }
    wsTransform = *newEntityWSTransform;
    return true;
}

void Gizmo::ContextDeleter::operator()(void* ctx) const {
    ImGuizmo::DestroyContext(static_cast<ImGuizmo::Context*>(ctx));
}
