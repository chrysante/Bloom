#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <scatha/Common/Ranges.h>
#include <scatha/Sema/Entity.h>
#include <scatha/Sema/SymbolTable.h>
#include <utl/typeinfo.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Graphics/Lights.h"
#include "Bloom/Runtime/ScriptSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/EntitySelectionManager.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/BasicSceneInspector.h"

using namespace vml::short_types;
using namespace ranges::views;
using namespace bloom;
using namespace poppy;

namespace {

class EntityInspector: public View, public BasicSceneInspector {
public:
    EntityInspector(): BasicSceneInspector(this) {}

private:
    void display() override;

    void inspectTag(EntityHandle);
    template <typename... Components>
    void addComponentButton(char const* name, EntityHandle entity,
                            bool forceDisable = false);

    void inspectTransform(EntityHandle);

    void inspectMesh(EntityHandle);
    void receiveMeshDragDrop(EntityHandle);
    void receiveMaterialDragDrop(EntityHandle);

    void inspectLight(EntityHandle);

    void inspectLightCommon(auto& light, LightType type);
    void inspectLightComponent(PointLight&);
    void inspectLightComponent(SpotLight&);
    void inspectLightComponent(DirectionalLight&);
    void inspectLightComponent(SkyLight&);

    void inspectPhysics(EntityHandle entity);
    void inspectColliderComponent(SphereColliderComponent&);
    void inspectColliderComponent(BoxColliderComponent&);

    void inspectScript(EntityHandle);

    // Helpers
    bool beginComponentSection(std::string_view name);
    template <typename>
    bool beginComponentSection(std::string_view name, EntityHandle);
    bool beginSubSection(std::string_view name);

    bool beginGenericSection(std::string_view name, FontDesc const&,
                             utl::function<void()> deleter);

    void endSection();
    void endSubSection();

    int editingNameState = 0;
    bool transformScaleLinked = false;
};

} // namespace

POPPY_REGISTER_VIEW(EntityInspector, "Entity Inspector");

void EntityInspector::display() {
    if (scenes().empty()) {
        displayEmptyWithReason("No Active Scene");
        return;
    }
    auto entity = selection().empty() ? EntityHandle{} :
                                        selection().entities().front();
    if (!entity) {
        displayEmptyWithReason("No Entity Selected");
        return;
    }
    if (entity.has<TagComponent>()) {
        inspectTag(entity);
        ImGui::Separator();
    }
    if (entity.has<Transform>()) {
        inspectTransform(entity);
        ImGui::Separator();
    }
    if (entity.has<MeshRendererComponent>()) {
        inspectMesh(entity);
        ImGui::Separator();
    }
    if (hasLightComponent(entity)) {
        inspectLight(entity);
        ImGui::Separator();
    }
    if (entity.hasAny<PhysicsStaticComponent, PhysicsMovableComponent>()) {
        inspectPhysics(entity);
        ImGui::Separator();
    }
    if (entity.has<ScriptComponent>()) {
        inspectScript(entity);
        ImGui::Separator();
    }
}

void EntityInspector::inspectTag(EntityHandle entity) {
    auto& tag = entity.get<TagComponent>();
    std::string_view name = tag.name;
    float2 framePadding = GImGui->Style.FramePadding;
    ImGui::BeginChild("##inspect-tag-child",
                      { 0, GImGui->FontSize + 2 * framePadding.y });

    float2 spacing = GImGui->Style.ItemSpacing;
    withFont(FontWeight::Bold, FontStyle::Roman, [&] {
        float2 windowSize = ImGui::GetWindowSize();
        float2 addButtonSize = float2(ImGui::CalcTextSize("Add Component")) +
                               float2(4, 2) * framePadding;
        float2 nameTextSize = { windowSize.x - addButtonSize.x - spacing.x,
                                windowSize.y };
        if (editingNameState && !isSimulating()) {
            char buffer[256]{};
            std::strncpy(buffer, name.data(), 255);
            ImGui::SetNextItemWidth(nameTextSize.x);
            if (editingNameState > 1) {
                ImGui::SetKeyboardFocusHere();
            }
            if (ImGui::InputText("##name-input", buffer, 256)) {
                tag.name = buffer;
            }
            editingNameState = ImGui::IsWindowFocused();
        }
        else {
            ImGui::SetCursorPos(framePadding);
            ImGui::SetNextItemWidth(nameTextSize.x);
            ImGui::Text("%s", name.data());
            ImGui::SetCursorPos({});
            editingNameState =
                ImGui::InvisibleButton("activate-name-input", nameTextSize,
                                       ImGuiButtonFlags_PressedOnDoubleClick) *
                2;
        }
        /// "Add Component" button
        disabledIf(isSimulating(), [&] {
            ImGui::SetCursorPos({ nameTextSize.x + spacing.x, 0 });
            ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                  Appearance::get()
                                      .style()
                                      .colors.highlightControlFrame);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                                  Appearance::get()
                                      .style()
                                      .colors.highlightControlFrameHovered);
            ImGui::PushStyleColor(ImGuiCol_Text, 0xFFffFFff);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                float2(2, 1) * framePadding);
            ImGui::SetNextItemWidth(addButtonSize.x);
            bool comboOpen = ImGui::BeginCombo("##-add-component",
                                               "Add Component",
                                               ImGuiComboFlags_NoArrowButton);
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            if (comboOpen) {
                withFont(FontDesc::UIDefault(), [&] {
                    bool hasLight = hasLightComponent(entity);
                    bool hasCollider = hasColliderComponent(entity);
                    addComponentButton<MeshRendererComponent>("Mesh Renderer",
                                                              entity);
                    addComponentButton<PointLightComponent>("Point Light",
                                                            entity, hasLight);
                    addComponentButton<SpotLightComponent>("Spot Light", entity,
                                                           hasLight);
                    addComponentButton<DirectionalLightComponent>(
                        "Directional Light", entity, hasLight);
                    addComponentButton<SkyLightComponent>("Sky Light", entity,
                                                          hasLight);
                    addComponentButton<PhysicsStaticComponent,
                                       SphereColliderComponent>(
                        "Sphere Collider", entity, hasCollider);
                    addComponentButton<PhysicsStaticComponent,
                                       BoxColliderComponent>("Box Collider",
                                                             entity,
                                                             hasCollider);
                    addComponentButton<ScriptComponent,
                                       ScriptPreservedData>("Script", entity);
                });
                ImGui::EndCombo();
            }
        });
    });
    ImGui::EndChild();
}

template <typename... Components>
void EntityInspector::addComponentButton(char const* name, EntityHandle entity,
                                         bool forceDisable) {
    int flags = 0;
    if (entity.hasAll<Components...>() || forceDisable) {
        flags |= ImGuiSelectableFlags_Disabled;
    }
    if (ImGui::Selectable(name, false, flags)) {
        (entity.tryAdd<Components>(), ...);
    }
}

void EntityInspector::inspectTransform(EntityHandle entity) {
    using namespace propertiesView;
    auto& transform = entity.get<Transform>();
    if (beginComponentSection("Transform")) {
        disabledIf(isSimulating(), [&] {
            auto& style = ImGui::GetStyle();
            beginProperty("Position");
            float2 lockButtonSize =
                ImGui::GetFontSize() + style.FramePadding.y * 2;
            float width = ImGui::GetContentRegionAvail().x - lockButtonSize.x -
                          style.ItemSpacing.x;
            ImGui::SetNextItemWidth(width);
            dragFloat3Labelled("###position", transform.position.data());
            beginProperty("Orientation");
            float3 euler = vml::to_euler(transform.orientation) * 180;
            ImGui::SetNextItemWidth(width);
            if (dragFloat3Labelled("###orientation", euler.data(),
                                   { .format = "%.3f°" }))
            {
                transform.orientation = vml::to_quaternion(euler / 180);
            }
            beginProperty("Scale");
            ImGui::SetNextItemWidth(width);
            float3 newScale = transform.scale;
            if (dragFloat3Labelled("###scale", newScale.data(),
                                   { .speed = 0.02,
                                     .linkFields = transformScaleLinked }))
            {
                transform.scale = newScale;
            }
            ImGui::SameLine();
            linkButton(transformScaleLinked, lockButtonSize);
        });
        endSection();
    }
}

void EntityInspector::inspectMesh(EntityHandle entity) {
    using namespace propertiesView;
    if (!beginComponentSection<MeshRendererComponent>("Mesh Renderer", entity))
    {
        return;
    }
    auto& meshRenderer = entity.get<MeshRendererComponent>();
    disabledIf(isSimulating(), [&] {
        using enum AssetRepresentation;
        beginProperty("Mesh");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        assetButton("###Mesh", meshRenderer.mesh, { .repr = GPU });
        beginProperty("Material");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        assetButton("###MaterialInstance", meshRenderer.materialInstance,
                    { .repr = GPU });
    });
    endSection();
}

template <typename E>
static std::optional<E> myEnumCombo(char const* label, E value) {
    auto myToString = [](E e) {
        if constexpr (requires { toString(e); }) {
            return std::string(toString(e));
        }
        else {
            return std::string(magic_enum::enum_name(e));
        }
    };
    std::optional<E> newValue;
    if (ImGui::BeginCombo(label, myToString(value).data())) {
        for (auto current: magic_enum::enum_values<E>()) {
            bool selected = value == current;
            if (ImGui::Selectable(myToString(current).data(), selected) &&
                !selected)
            {
                newValue = current;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return newValue;
}

template <typename E>
static std::optional<E> inspectTypeGeneric(char const* ID, E type,
                                           auto exchange) {
    auto newType = myEnumCombo(ID, type);
    if (newType) {
        std::invoke(exchange, *newType);
    }
    return newType;
}

void EntityInspector::inspectLight(EntityHandle entity) {
    auto type = getLightType(entity).value();
    auto deleter = [&] {
        dispatchComponentType(type, [&]<typename L>(utl::tag<L>) {
            entity.remove<L>();
        });
    };
    auto font = FontDesc::UIDefault().setWeight(FontWeight::Semibold);
    if (!beginGenericSection("Light Component", font, deleter)) {
        return;
    }
    disabledIf(isSimulating(), [&] {
        using namespace propertiesView;
        beginProperty("Type");
        fullWidth();
        auto newType =
            inspectTypeGeneric("###Type", type, [&](LightType newType) {
            auto color =
                dispatchComponentType(type, [&]<typename C>(utl::tag<C>) {
                return entity.get<C>().color;
            });
            dispatchComponentType(newType, [&]<typename C>(utl::tag<C>) {
                auto& c = entity.add(C{ C::Light::Default() });
                c.color = color;
            });
            removeComponentType(type, entity);
        });
        type = newType.value_or(type);
        dispatchComponentType(type, [&]<typename L>(utl::tag<L>) {
            inspectLightComponent(entity.get<L>());
        });
    });
    endSection();
}

void EntityInspector::inspectLightCommon(auto& light, LightType type) {
    using namespace propertiesView;
    beginProperty("Color");
    ImGui::ColorEdit3("##light-color", light.color.data(),
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_Float |
                          ImGuiColorEditFlags_NoAlpha |
                          ImGuiColorEditFlags_PickerHueWheel);
    beginProperty("Intensity");
    float speed = type == LightType::DirectionalLight ? 0.01 :
                  type == LightType::SkyLight         ? 0.0001 :
                                                        100;
    fullWidth();
    ImGui::DragFloat("intensity", &light.intensity, speed, 0, FLT_MAX, "%f");
}

void EntityInspector::inspectLightComponent(PointLight& light) {
    using namespace propertiesView;
    inspectLightCommon(light, LightType::PointLight);
    beginProperty("Radius");
    fullWidth();
    ImGui::SliderFloat("radius", &light.radius, 0, 100);
    beginProperty("Falloff");
    fullWidth();
    ImGui::DragFloat("falloff", &light.falloff,
                     /* speed: */ 0.01, /* min: */ 0.01, /* max: */ FLT_MAX);
}

void EntityInspector::inspectLightComponent(SpotLight& light) {
    using namespace propertiesView;
    inspectLightCommon(light, LightType::SpotLight);
    beginProperty("Radius");
    fullWidth();
    ImGui::SliderFloat("radius", &light.radius, 0, 100);
    float inner = light.innerCutoff;
    float outer = light.outerCutoff;
    float angle = (inner + outer) / 2;
    float falloff = (outer - inner) / 2;
    beginProperty("Angle");
    fullWidth();
    ImGui::SliderFloat("angle", &angle, 0, 1);
    beginProperty("Falloff");
    fullWidth();
    ImGui::SliderFloat("falloff", &falloff, 0, 0.2);
    light.innerCutoff = angle - falloff;
    light.outerCutoff = angle + falloff;
}

void EntityInspector::inspectLightComponent(DirectionalLight& light) {
    using namespace propertiesView;
    inspectLightCommon(light, LightType::DirectionalLight);
    beginProperty("Source Size");
    fullWidth();
    ImGui::DragFloat("source-size", &light.sourceSize, 0.01, 0, FLT_MAX);
    beginProperty("Casts Shadow");
    ImGui::Checkbox("##-casts-shadow", &light.castsShadows);
    if (!light.castsShadows) {
        return;
    }
    beginProperty("Shadow Distance");
    fullWidth();
    ImGui::DragFloat("shadow-distance", &light.shadowDistance, 1, 0, FLT_MAX);
    beginProperty("Shadow Z Distance");
    fullWidth();
    ImGui::DragFloat("shadow-distance-z", &light.shadowDistanceZ, 1, 0,
                     FLT_MAX);
    beginProperty("Number Of Cascades");
    int nc = light.numCascades;
    fullWidth();
    ImGui::SliderInt("num-cascades", &nc, 1, 10);
    light.numCascades = nc;
    beginProperty("Distribution Exponent");
    fullWidth();
    ImGui::SliderFloat("cascade-distribution-exponent",
                       &light.cascadeDistributionExponent, 1, 4);
    beginProperty("Transition");
    fullWidth();
    ImGui::SliderFloat("cascade-transition-fraction",
                       &light.cascadeTransitionFraction, 0, 1);
    beginProperty("Fadeout");
    fullWidth();
    ImGui::SliderFloat("shadow-distance-fadeout-fraction",
                       &light.shadowDistanceFadeoutFraction, 0, 1);
}

void EntityInspector::inspectLightComponent(SkyLight& light) {
    inspectLightCommon(light, LightType::SkyLight);
    using namespace propertiesView;
    beginProperty("IBL Texture");
    fullWidth();
    using enum AssetRepresentation;
    assetButton("###IBLTexture", light.cubemap, { .repr = GPU | CPU });
}

void EntityInspector::inspectPhysics(EntityHandle entity) {
    using namespace propertiesView;
    auto physicsType = getPhysicsType(entity).value();
    auto colliderType = getColliderType(entity).value();
    auto deleter = [&] {
        dispatchComponentType(physicsType, [&]<typename M>(utl::tag<M>) {
            entity.remove<M>();
        });
        dispatchComponentType(colliderType, [&]<typename L>(utl::tag<L>) {
            entity.remove<L>();
        });
    };
    auto font = FontDesc::UIDefault().setWeight(FontWeight::Semibold);
    if (!beginGenericSection("Physics", font, deleter)) {
        return;
    }
    disabledIf(isSimulating(), [&] {
        beginProperty("Movability");
        fullWidth();
        physicsType = inspectTypeGeneric("###Movability", physicsType,
                                         [&](PhysicsComponentType newType) {
            dispatchComponentType(newType, [&]<typename C>(utl::tag<C>) {
                entity.add<C>();
            });
            removeComponentType(physicsType, entity);
        }).value_or(physicsType);
        fullWidth();
        beginProperty("Collider");
        fullWidth();
        colliderType = inspectTypeGeneric("###Collider", colliderType,
                                          [&](ColliderType newType) {
            dispatchComponentType(newType, [&]<typename C>(utl::tag<C>) {
                entity.add(C{});
            });
            removeComponentType(colliderType, entity);
        }).value_or(colliderType);
        dispatchComponentType(colliderType, [&]<typename C>(utl::tag<C>) {
            inspectColliderComponent(entity.get<C>());
        });
    });
    endSection();
}

void EntityInspector::inspectColliderComponent(
    SphereColliderComponent& collider) {
    using namespace propertiesView;
    beginProperty("Radius");
    fullWidth();
    ImGui::DragFloat("###Radius", &collider.radius);
}

void EntityInspector::inspectColliderComponent(BoxColliderComponent& collider) {
    using namespace propertiesView;
    beginProperty("Half Extend");
    fullWidth();
    dragFloat3Labelled("###HalfExtend", collider.halfExtend.data(),
                       { .showLinkButton = true,
                         .labelStyle = DragFloat3Options::Slim });
}

void EntityInspector::inspectScript(EntityHandle entity) {
    using namespace propertiesView;
    if (!entity.has<ScriptPreservedData>()) {
        ImGui::Text("Error: No preserved data");
        return;
    }
    auto& scriptSystem = editor().coreSystems().scriptSystem();
    auto* sym = scriptSystem.symbolTable();
    auto& component = entity.get<ScriptComponent>();
    auto& preservedData = entity.get<ScriptPreservedData>();
    if (!sym) {
        ImGui::Text("Error: No symbol table");
        return;
    }
    if (!beginComponentSection<ScriptComponent>("Script Component", entity)) {
        return;
    }
    disabledIf(isSimulating(), [&] {
        beginProperty("Class");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        char const* name = component.type ? component.type->name().data() : "";
        if (ImGui::BeginCombo("##-script", name)) {
            auto structTypes = sym->globalScope().entities() |
                               scatha::Filter<scatha::sema::StructType>;
            for (auto* type: structTypes) {
                bool selected = type == component.type;
                if (ImGui::Selectable(type->name().data(), selected)) {
                    assignType(component, type);
                    preservedData.classname = std::string(type->name());
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
#if 0
        if (!script.objectAddress == 0 || !beginSubSection("Attributes")) {
            return;
        }
        auto members = script.classType->memberVariables();
        for (auto* var: members) {
            beginProperty(var->name().data());
            // clang-format off
            visit(*var->type()->base(), utl::overload{
                [&](scatha::sema::FloatType const& type) {
                    auto name = var->name();
                    auto* data = (char*)script.object + var->offset();
                    switch (type.bitwidth()) {
                    case 32:
                        editDataField(name, (float*)data);
                        break;
                    case 64:
                        editDataField(name, (double*)data);
                        break;
                    default:
                        assert(false);
                    }
                },
                [&](scatha::sema::ObjectType const& type) {
                    /// Unimplemented
                }
            }); // clang-format on
        }
        endSubSection();
#endif
    });
    endSection();
}

/// MARK: - Helpers
bool EntityInspector::beginComponentSection(std::string_view name) {
    return beginComponentSection<void>(name, EntityHandle{});
}
template <typename T>
bool EntityInspector::beginComponentSection(std::string_view name,
                                            EntityHandle entity) {
    if constexpr (std::is_same_v<T, void>) {
        return beginGenericSection(name,
                                   FontDesc::UIDefault().setWeight(
                                       FontWeight::Semibold),
                                   nullptr);
    }
    else {
        return beginGenericSection(name,
                                   FontDesc::UIDefault().setWeight(
                                       FontWeight::Semibold),
                                   [&] { entity.remove<T>(); });
    }
}

bool EntityInspector::beginSubSection(std::string_view name) {
    endSection();
    return beginGenericSection(name,
                               FontDesc::UIDefault().setStyle(
                                   FontStyle::Italic),
                               nullptr);
}

bool EntityInspector::beginGenericSection(std::string_view name,
                                          FontDesc const& font,
                                          utl::function<void()> deleter) {
    using namespace propertiesView;
    float2 const cursorPos = ImGui::GetCursorPos();
    header(name, font);
    bool const open = [&] {
        if (!deleter) {
            return true;
        }
        // 'Delete' Button
        return withFont(FontWeight::Semibold, FontStyle::Roman, [&] {
            return disabledIf(isSimulating(), [&] {
                bool result = true;
                float2 const textSize = ImGui::CalcTextSize("Delete");
                float2 const buttonPos = {
                    cursorPos.x + ImGui::GetContentRegionAvail().x - textSize.x,
                    cursorPos.y
                };
                ImGui::SetCursorPos(buttonPos);
                char deleteButtonLabel[64] = "-delete-button";
                std::strncpy(&deleteButtonLabel[14], name.data(), 49);
                if (ImGui::InvisibleButton(deleteButtonLabel, textSize)) {
                    deleter();
                    result = false;
                }
                float4 const color =
                    GImGui->Style.Colors[ImGuiCol_TextDisabled];
                float4 const hightlightColor =
                    (float4(GImGui->Style.Colors[ImGuiCol_Text]) + color) / 2;
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::IsItemHovered() ?
                                                         hightlightColor :
                                                         color);
                ImGui::SetCursorPos(buttonPos);
                ImGui::Text("Delete");
                ImGui::PopStyleColor();
                return result;
            });
        });
    }();
    if (open) {
        beginSection();
        return true;
    }
    else {
        return false;
    }
}

void EntityInspector::endSection() { propertiesView::endSection(); }

void EntityInspector::endSubSection() {
    endSection();
    propertiesView::beginSection();
}
