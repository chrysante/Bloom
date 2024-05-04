#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include <optional>
#include <span>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/function_view.hpp>
#include <utl/hashset.hpp>
#include <utl/scope_guard.hpp>
#include <utl/stack.hpp>
#include <utl/utility.hpp>
#include <utl/vector.hpp>

#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Entity.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/EntitySelectionManager.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/BasicSceneInspector.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

namespace {

class SceneOutliner: public View, public BasicSceneInspector {
public:
    SceneOutliner(): BasicSceneInspector(this) {}

private:
    void display() override;
    void onInput(InputEvent&) override;

    struct TreeNodeDescription {
        size_t id = 0;
        bool selected = false;
        bool expanded = false;
        bool isLeaf = true;
        std::string_view name;
    };
    bool treeNode(TreeNodeDescription const& desc);
    void displayScene(Scene& scene);
    void sceneContextMenu(Scene& scene);
    void displayEntity(EntityHandle entity);
    void entityContextMenu(EntityHandle entity);

    void dragDropSource(EntityHandle child);
    void dragDropTarget(EntityHandle parent);

    bool expanded(Scene const*) const;
    bool expanded(ConstEntityHandle) const;
    void setExpanded(Scene const*, bool);
    void setExpanded(ConstEntityHandle, bool);

private:
    utl::hashset<ConstEntityHandle> mExpanded;
};

} // namespace

POPPY_REGISTER_VIEW(SceneOutliner, "Scene Outliner");

void SceneOutliner::display() {
    if (scenes().empty()) {
        displayEmptyWithReason("No Active Scene");
        return;
    }
    if (isSimulating()) {
        displayEmptyWithReason("Simulation Running");
        return;
    }
    for (auto scene: scenes()) {
        displayScene(*scene);
    }
}

void SceneOutliner::onInput(InputEvent&) {}

[[maybe_unused]] static void eyeButton() {
    auto& style = ImGui::GetStyle();
    ImGui::SetCursorPosX(style.WindowPadding.x + style.FramePadding.x);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, float2());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, float2());
    auto icon = FontManager::get({ IconSize::_16 }, "eye");
    ImGui::PushFont(icon.font);
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::Button(icon.utf8Rep.c_str());
    ImGui::PopStyleColor(4);
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
}

bool SceneOutliner::treeNode(TreeNodeDescription const& desc) {
    ImGuiTreeNodeFlags flags = 0;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= ImGuiTreeNodeFlags_SpanFullWidth;
    flags |= ImGuiTreeNodeFlags_FramePadding;
    if (desc.selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (desc.expanded) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (desc.isLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
    bool nodeExpanded =
        ImGui::TreeNodeEx((void*)desc.id, flags, "%s", desc.name.data());
    ImGui::PopStyleVar();
    return nodeExpanded;
}

void SceneOutliner::displayScene(Scene& scene) {
    bool isExpanded = treeNode({ .name = scene.name(),
                                 .id = scene.hashID(),
                                 .selected = false,
                                 .expanded = expanded(&scene),
                                 .isLeaf = false });
    sceneContextMenu(scene);
    setExpanded(&scene, isExpanded);
    dragDropTarget(EntityHandle(EntityID{}, &scene));
    if (isExpanded) {
        for (auto id: scene.gatherRoots()) {
            displayEntity(scene.getHandle(id));
        }
        ImGui::TreePop();
    }
}

void SceneOutliner::sceneContextMenu(Scene& scene) {
    ImGui::PushID((void*)scene.hashID());
    utl_defer { ImGui::PopID(); };
    ImGui::OpenPopupOnItemClick("Context Menu");
    if (!ImGui::BeginPopup("Context Menu")) {
        return;
    }
    utl_defer { ImGui::EndPopup(); };
    if (ImGui::BeginMenu("New Entity")) {
        if (ImGui::MenuItem("Mesh Renderer")) {
            auto entity = scene.createEntity("Mesh Renderer");
            entity.add(MeshRendererComponent{});
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Point Light")) {
            auto entity = scene.createEntity("Point Light");
            entity.add(PointLightComponent{});
        }
        if (ImGui::MenuItem("Spotlight")) {
            auto entity = scene.createEntity("Spotlight");
            entity.add(SpotLightComponent{});
        }
        if (ImGui::MenuItem("Directional Light")) {
            auto entity = scene.createEntity("Directional Light");
            entity.add(DirectionalLightComponent{});
        }
        if (ImGui::MenuItem("Sky Light")) {
            auto entity = scene.createEntity("Sky Light");
            entity.add(SkyLightComponent{});
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Empty Entity")) {
            scene.createEntity("Entity");
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Unload")) {
        editor().coreSystems().sceneSystem().unloadScene(scene.handle().ID());
        return;
    }
}

void SceneOutliner::displayEntity(EntityHandle entity) {
    BL_ASSERT(entity);
    bool isExpanded = treeNode({ .name = entity.get<TagComponent>().name,
                                 .id = entity.raw(),
                                 .selected = selection().contains(entity),
                                 .expanded = expanded(entity),
                                 .isLeaf = entity.scene().isLeaf(entity) });
    entityContextMenu(entity);
    setExpanded(entity, isExpanded);
    dragDropSource(entity);
    dragDropTarget(entity);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selection().amend(entity, getSelectionMode());
    }
    if (isExpanded) {
        for (auto c: entity.scene().gatherChildren(entity)) {
            displayEntity(entity.scene().getHandle(c));
        }
        ImGui::TreePop();
    }
}

void SceneOutliner::entityContextMenu(EntityHandle entity) {
    ImGui::PushID((int)entity.raw());
    utl_defer { ImGui::PopID(); };
    ImGui::OpenPopupOnItemClick("Context Menu");
    if (ImGui::BeginPopup("Context Menu")) {
        utl_defer { ImGui::EndPopup(); };
        if (ImGui::MenuItem("Delete Entity")) {
            dispatch(DispatchToken::NextFrame, [this, entity] {
                entity.scene().deleteEntity(entity);
                selection().erase(entity);
            });
        }
        if (entity.getParent() && ImGui::MenuItem("Unparent")) {
            dispatch(DispatchToken::NextFrame, [entity] { entity.unparent(); });
        }
    }
}

void SceneOutliner::dragDropSource(EntityHandle child) {
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DD-Entity-Hierarchy-View", &child,
                                  sizeof child);
        displayEntity(child);
        ImGui::EndDragDropSource();
    }
}

void SceneOutliner::dragDropTarget(EntityHandle parent) {
    using namespace bloom;
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }
    auto* payload = ImGui::AcceptDragDropPayload("DD-Entity-Hierarchy-View");
    if (!payload || !payload->IsDelivery()) {
        ImGui::EndDragDropTarget();
        return;
    }
    EntityHandle child;
    std::memcpy(&child, payload->Data, sizeof child);
    dispatch(DispatchToken::NextFrame,
             [this, child, parent, window = ImGui::GetCurrentWindow()] {
        if (&child.scene() != &parent.scene()) {
            return;
        }
        /// We need to make sure that we don't attach an entity to it's own
        /// descendend
        auto& scene = child.scene();
        if (!child || scene.descendsFrom(parent, child)) {
            Logger::Warn("Hierarchy Error");
            return;
        }
        child.unparent();
        if (parent) {
            scene.parent(child, parent);
            setExpanded(parent, true);
        }
    });
    ImGui::EndDragDropTarget();
}

bool SceneOutliner::expanded(Scene const* scene) const {
    return expanded(ConstEntityHandle(EntityID{}, scene));
}

bool SceneOutliner::expanded(ConstEntityHandle entity) const {
    return mExpanded.contains(entity);
}

void SceneOutliner::setExpanded(Scene const* scene, bool expanded) {
    setExpanded(ConstEntityHandle(EntityID{}, scene), expanded);
}

void SceneOutliner::setExpanded(ConstEntityHandle entity, bool expanded) {
    if (expanded) {
        mExpanded.insert(entity);
    }
    else {
        mExpanded.erase(entity);
    }
}
