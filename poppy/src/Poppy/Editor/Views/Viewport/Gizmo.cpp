#include "Poppy/Editor/Views/Viewport/Gizmo.h"

#include <array>

#include <ImGuizmo.h>
#include <imgui.h>
#include <mtl/mtl.hpp>

#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/SelectionContext.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

std::string poppy::toString(Gizmo::Operation op) {
    return std::array<std::string, 3>{ "Translate", "Rotate",
                                       "Scale" }[(std::size_t)op];
}
std::string poppy::toString(Gizmo::Space sp) {
    return std::array<std::string, 2>{ "World", "Local" }[(std::size_t)sp];
}

Gizmo::Gizmo():
    context(reinterpret_cast<ImGuizmoCtx*>(ImGuizmo::CreateContext())) {}

void Gizmo::setOperation(Operation op) {
    if (isUsing()) {
        // Guard here because of a bug in ImGuizmo. We crash when changing
        // operation while using gizmo.
        return;
    }
    _operation = op;
}

void Gizmo::setSpace(Space sp) {
    if (_operation == Operation::Scale) {
        return;
    }
    _space[(std::size_t)_operation] = sp;
}

void Gizmo::cycleSpace() {
    auto const newSpace = space() == Space::World ? Space::Local : Space::World;
    setSpace(newSpace);
}

static std::pair<bool, mtl::float4x4> manipulateGizmo(
    ImGuizmo::Context* context, mtl::float4x4 view, mtl::float4x4 proj,
    Gizmo::Operation operation, Gizmo::Space space, mtl::float4x4 transform) {
    ImGuizmo::SetContext(context);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    float2 const pos = ImGui::GetWindowPos();
    float2 const size = ImGui::GetWindowSize();
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    view = mtl::transpose(view);
    proj = mtl::transpose(proj);
    transform = mtl::transpose(transform);

    auto const imguizmoOperation = (ImGuizmo::OPERATION)operation;
    auto const imguizmoMode = operation == Gizmo::Operation::Scale ?
                                  ImGuizmo::MODE::LOCAL :
                                  (ImGuizmo::MODE)(1 - (int)space);

    if (ImGuizmo::Manipulate(view.data(), proj.data(), imguizmoOperation,
                             imguizmoMode, transform.data()))
    {
        return { true, mtl::transpose(transform) };
    }
    return { false, 0 };
}

void Gizmo::display(bloom::Camera const& camera, SelectionContext& selection) {
    if (selection.empty()) {
        return;
    }

    EntityHandle entity = *selection.entities().begin();
    bloom::Transform const transformComponent = entity.get<Transform>();
    EntityID const parent = entity.has<HierarchyComponent>() ?
                                entity.get<HierarchyComponent>().parent :
                                EntityID{};

    auto& scene = entity.scene();

    auto const view = camera.view();
    auto const proj = camera.projection();
    auto const localTransform = transformComponent.calculate();
    auto const parentTransform =
        scene.calculateTransformRelativeToWorld(parent);

    auto const entityWSTransform = parentTransform * localTransform;

    auto const [manipulated, newEntityWSTransform] =
        manipulateGizmo(reinterpret_cast<ImGuizmo::Context*>(context.get()),
                        view, proj, operation(), space(), entityWSTransform);

    _hovered = ImGuizmo::IsOver();

    bool const usedLastFrame = _using;
    _using = manipulated ||
             (usedLastFrame && _input->mouseDown(MouseButton::Left));
    if (!manipulated) {
        return;
    }

    if (!usedLastFrame && _input && _input->keyDown(Key::LeftShift)) {
        entity = scene.cloneEntity(entity);
        selection.select(entity);
    }

    auto const newLocalTransform =
        mtl::inverse(parentTransform) * newEntityWSTransform;

    entity.get<Transform>() = Transform::fromMatrix(newLocalTransform);
}

void Gizmo::ImGuizmoDeleter::operator()(ImGuizmoCtx* ctx) const {
    ImGuizmo::DestroyContext(reinterpret_cast<ImGuizmo::Context*>(ctx));
}
