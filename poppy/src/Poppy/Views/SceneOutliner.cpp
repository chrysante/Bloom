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

struct TreeNodeDescription {
    size_t id = 0;
    bool selected = false;
    bool expanded = false;
    bool isLeaf = true;
    bool haveEyeButton = false;
    bool eyeButtonCrossed = false;
    bool entityVisible = true; /// This differs from `eyeButtonCrossed` because
                               /// a parent entity may be hidden
    std::string_view name;
    utl::function_view<void()> itemCallback = {};
};

struct TreeNodeResult {
    bool isExpanded;
    bool isClicked;
    bool isToggledOpen;
    bool isEyeButtonPressed;
};

struct SceneOutliner: View, BasicSceneInspector {
    SceneOutliner(): BasicSceneInspector(this) {}

    void init() override;

    void display() override;

    TreeNodeResult treeNode(TreeNodeDescription const& desc);
    void displayScene(Scene& scene);
    void sceneContextMenu(Scene& scene);
    void displayEntity(EntityHandle entity);
    void entityContextMenu(EntityHandle entity, ImGuiID popupID);

    void dragDropSource(EntityHandle child);
    void dragDropTarget(EntityHandle parent);

    bool expanded(Scene const*) const;
    bool expanded(ConstEntityHandle) const;
    void setExpanded(Scene const*, bool);
    void setExpanded(ConstEntityHandle, bool);

    utl::hashset<ConstEntityHandle> mExpanded;
};

} // namespace

POPPY_REGISTER_VIEW(SceneOutliner, "Scene Outliner");

void SceneOutliner::init() {
    editor().reactToSceneCommands(commandPaletteWrite());
}

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
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        selection().clear();
    }
}

static bool eyeButton(float2 nodePosLocal, bool selectedOrHovered,
                      bool disabled, bool crossed) {
    auto& g = *GImGui;
    auto& style = ImGui::GetStyle();
    float2 stashedPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorPos({ ImGui::GetCursorStartPos().x, nodePosLocal.y });
    float2 pos = ImGui::GetCursorScreenPos();
    float2 size(g.FontSize + 2 * style.FramePadding.y);
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    ImGui::Button("###EyeButton", { size.x, size.y + 1 });
    bool pressed = ImGui::IsItemClicked();
    ImGui::PopStyleColor(5);
    auto* DL = ImGui::GetWindowDrawList();
    auto eyeCol = selectedOrHovered ?
                      IM_COL32_WHITE :
                      ImGui::GetColorU32(disabled ? ImGuiCol_TextDisabled :
                                                    ImGuiCol_Text);
    auto icon =
        FontManager::get({ IconSize::_16 }, crossed ? "eye-off" : "eye");
    float iconSize = 0.9;
    DL->AddText(icon.font, g.FontSize * iconSize,
                pos + (float2)style.FramePadding +
                    float2(0, g.FontSize * 0.5 * (1 - iconSize)),
                eyeCol, icon.utf8Rep.c_str());
    ImGui::SetCursorScreenPos(stashedPos);
    return pressed;
}

TreeNodeResult SceneOutliner::treeNode(TreeNodeDescription const& desc) {
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
    float2 pos = ImGui::GetCursorPos();
    ImGui::SetNextItemAllowOverlap();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::GetStyleColorVec4(!desc.entityVisible ?
                                                       ImGuiCol_TextDisabled :
                                                       ImGuiCol_Text));
    bool isExpanded =
        ImGui::TreeNodeEx((void*)desc.id, flags, "%s", desc.name.data());
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    if (desc.itemCallback) {
        desc.itemCallback();
    }
    TreeNodeResult result = {
        .isExpanded = isExpanded,
        .isClicked = ImGui::IsItemClicked(),
        .isToggledOpen = ImGui::IsItemToggledOpen(),
    };
    if (desc.haveEyeButton) {
        result.isEyeButtonPressed =
            eyeButton(pos, ImGui::IsItemHovered() || desc.selected,
                      /* disabled: */ !desc.entityVisible,
                      /* crossed: */ desc.eyeButtonCrossed);
    }
    return result;
}

void SceneOutliner::displayScene(Scene& scene) {
    bool isExpanded = treeNode({ .name = scene.name(),
                                 .id = scene.hashID(),
                                 .selected = false,
                                 .expanded = expanded(&scene),
                                 .isLeaf = false,
                                 .haveEyeButton = false })
                          .isExpanded;
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
        dispatch(DispatchToken::NextFrame, [this, ID = scene.handle().ID()] {
            editor().coreSystems().sceneSystem().unloadScene(ID);
        });
        return;
    }
}

void SceneOutliner::displayEntity(EntityHandle entity) {
    BL_ASSERT(entity);
    auto* vis = entity.tryGet<VisibilityComponent>();
    ImGuiID popupID = ImGui::GetCurrentWindow()->GetID(entity.raw()) +
                      ImGui::GetID("ContextMenu");
    auto itemCallback = [&] {
        entityContextMenu(entity, popupID);
        dragDropSource(entity);
        dragDropTarget(entity);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            if (ImGui::BeginTooltip()) {
                ImGui::Text("Volatile ID: %d", entity.raw());
                ImGui::Text("Persistent ID: %s",
                            entity.persistentID().to_string().c_str());
                ImGui::EndTooltip();
            }
        }
    };
    auto [isExpanded, isClicked, isToggledOpen, isEyeButtonPressed] =
        treeNode({ .name = entity.get<TagComponent>().name,
                   .id = entity.raw(),
                   .selected = selection().contains(entity),
                   .expanded = expanded(entity),
                   .isLeaf = entity.isLeaf(),
                   .haveEyeButton = vis != nullptr,
                   .eyeButtonCrossed = vis && !vis->value,
                   .entityVisible = entity.isVisible(),
                   .itemCallback = itemCallback });
    setExpanded(entity, isExpanded);
    if (isClicked && !isToggledOpen) {
        selection().amend(entity, getSelectionMode());
    }
    if (isExpanded) {
        for (auto c: entity.gatherChildren()) {
            displayEntity(entity.scene()->getHandle(c));
        }
        ImGui::TreePop();
    }
    if (isEyeButtonPressed) {
        vis->value ^= true;
    }
}

void SceneOutliner::entityContextMenu(EntityHandle entity, ImGuiID popupID) {
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopupEx(popupID);
    }
    auto flags = ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    if (!ImGui::BeginPopupEx(popupID, flags)) {
        return;
    }
    if (ImGui::MenuItem("Delete Entity")) {
        dispatch(DispatchToken::NextFrame, [this, entity] {
            entity.deleteEntity();
            selection().erase(entity);
        });
    }
    if (entity.getParent() && ImGui::MenuItem("Unparent")) {
        dispatch(DispatchToken::NextFrame,
                 [entity] { entity.detachFromHieararchy(); });
    }
    ImGui::EndPopup();
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
        if (child.scene() != parent.scene()) {
            return;
        }
        /// We need to make sure that we don't attach an entity to it's own
        /// descendend
        auto* scene = child.scene();
        if (!child || scene->descendsFrom(parent, child)) {
            Logger::Warn("Hierarchy Error");
            return;
        }
        child.detachFromHieararchy();
        if (parent) {
            scene->parent(child, parent);
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
