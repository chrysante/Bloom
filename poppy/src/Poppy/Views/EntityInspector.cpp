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
#include "Poppy/Editor/SceneTransactions.h"
#include "Poppy/Editor/TransactionManager.h"
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

    void inspectTag();
    template <typename... Components>
    void addComponentButton(char const* name, bool forceDisable = false);

    void inspectTransform();

    void inspectMesh();
    void receiveMeshDragDrop();
    void receiveMaterialDragDrop();

    void inspectLight();

    void inspectLightCommon(ComponentType auto& light, LightType type);
    void inspectLightComponent(PointLightComponent&);
    void inspectLightComponent(SpotLightComponent&);
    void inspectLightComponent(DirectionalLightComponent&);
    void inspectLightComponent(SkyLightComponent&);

    void inspectPhysics();
    void inspectColliderComponent(SphereColliderComponent&);
    void inspectColliderComponent(BoxColliderComponent&);

    void inspectScript();

    // Helpers
    bool beginComponentSection(std::string_view name);
    template <typename>
    bool beginComponentSection(std::string_view name);
    bool beginSubSection(std::string_view name);

    bool beginGenericSection(std::string_view name, FontDesc const&,
                             utl::function<void()> deleter);

    void endSection();
    void endSubSection();

    void editComponent(DataComponent auto const& component,
                       utl::function_view<void(TAToken&)> uiFunction) {
        poppy::editComponents(editor().transactionManager(), { { entity } },
                              std::array<EditEntityComponent::ConstPtrVariant,
                                         1>{ &component },
                              uiFunction);
    }

    EntityHandle entity;
    bool editingName = false;
    bool startEditingName = false;
    int editingNameState = 0;
    bool transformScaleLinked = false;
};

} // namespace

POPPY_REGISTER_VIEW(EntityInspector, "Entity Inspector");

void EntityInspector::display() {
    if (editor().isSimulating()) {
        displayEmptyWithReason("Simulation Running");
        return;
    }
    if (!scene()) {
        displayEmptyWithReason("No Active Scene");
        return;
    }
    if (selection().empty()) {
        displayEmptyWithReason("No Entity Selected");
        return;
    }
    if (selection().size() > 1) {
        displayEmptyWithReason(
            utl::strcat(selection().size(), " Entities Selected"));
        return;
    }
    entity = selection().entities().front();
    if (entity.has<TagComponent>()) {
        inspectTag();
        ImGui::Separator();
    }
    if (entity.has<Transform>()) {
        inspectTransform();
        ImGui::Separator();
    }
    if (entity.has<MeshRendererComponent>()) {
        inspectMesh();
        ImGui::Separator();
    }
    if (hasLightComponent(entity)) {
        inspectLight();
        ImGui::Separator();
    }
    if (entity.hasAny<PhysicsStaticComponent, PhysicsMovableComponent>()) {
        inspectPhysics();
        ImGui::Separator();
    }
    if (entity.has<ScriptComponent>()) {
        inspectScript();
        ImGui::Separator();
    }
}

void EntityInspector::inspectTag() {
    auto& tag = entity.get<TagComponent>();
    std::string_view name = tag.name;
    float2 framePadding = GImGui->Style.FramePadding;
    ImGui::BeginChild("##inspect-tag",
                      { 0, GImGui->FontSize + 2 * framePadding.y });
    float2 spacing = GImGui->Style.ItemSpacing;
    ImGui::PushFont(FontManager::get(FontWeight::Bold));
    float2 windowSize = ImGui::GetWindowSize();
    float2 addButtonSize = float2(ImGui::CalcTextSize("Add Component")) +
                           float2(4, 2) * framePadding;
    float2 nameTextSize = { windowSize.x - addButtonSize.x - spacing.x,
                            windowSize.y };
    if (editingName) {
        std::array<char, 1024> buffer;
        std::strncpy(buffer.data(), name.data(), buffer.size());
        ImGui::SetNextItemWidth(nameTextSize.x);
        if (startEditingName) {
            ImGui::SetKeyboardFocusHere();
            startEditingName = false;
        }
        editComponent(tag, [&](TAToken& token) {
            if (ImGui::InputText("##name-input", buffer.data(), buffer.size()))
            {
                tag.name = buffer.data();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                token.cancel();
            }
        });
        if (!ImGui::IsWindowFocused() || ImGui::IsItemDeactivated()) {
            editingName = false;
        }
    }
    else {
        ImGui::SetCursorPos(framePadding);
        ImGui::SetNextItemWidth(nameTextSize.x);
        ImGui::TextEx(name.data());
        ImGui::SetCursorPos({});
        if (ImGui::InvisibleButton("###EditName", nameTextSize,
                                   ImGuiButtonFlags_PressedOnDoubleClick))
        {
            editingName = true;
            startEditingName = true;
        }
    }
    /// "Add Component" button
    ImGui::SetCursorPos({ nameTextSize.x + spacing.x, 0 });
    auto& colors = Appearance::get().style().colors;
    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.highlightControlFrame);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                          colors.highlightControlFrameHovered);
    ImGui::PushStyleColor(ImGuiCol_Text, 0xFFffFFff);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        float2(2, 1) * framePadding);
    ImGui::SetNextItemWidth(addButtonSize.x);
    bool comboOpen = ImGui::BeginCombo("##-add-component", "Add Component",
                                       ImGuiComboFlags_NoArrowButton);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    if (comboOpen) {
        ImGui::PushFont(nullptr);
        bool hasLight = hasLightComponent(entity);
        bool hasCollider = hasColliderComponent(entity);
        addComponentButton<MeshRendererComponent>("Mesh Renderer");
        addComponentButton<PointLightComponent>("Point Light", hasLight);
        addComponentButton<SpotLightComponent>("Spot Light", hasLight);
        addComponentButton<DirectionalLightComponent>("Directional Light",
                                                      hasLight);
        addComponentButton<SkyLightComponent>("Sky Light", hasLight);
        addComponentButton<PhysicsStaticComponent,
                           SphereColliderComponent>("Sphere Collider",
                                                    hasCollider);
        addComponentButton<PhysicsStaticComponent,
                           BoxColliderComponent>("Box Collider", hasCollider);
        addComponentButton<ScriptComponent, ScriptPreservedData>("Script");
        ImGui::PopFont();
        ImGui::EndCombo();
    }
    ImGui::PopFont();
    ImGui::EndChild();
}

template <typename... Components>
void EntityInspector::addComponentButton(char const* name, bool forceDisable) {
    int flags = 0;
    if (entity.hasAll<Components...>() || forceDisable) {
        flags |= ImGuiSelectableFlags_Disabled;
    }
    if (ImGui::Selectable(name, false, flags)) {
        auto commit = [](EntityHandle entity) {
            return (entity.tryAdd<Components>(), ...);
        };
        auto undo = [](EntityHandle entity) {
            return (entity.tryRemove<Components>(), ...);
        };
        auto* transaction =
            new CustomEntityTransaction(entity.strongHandle(), commit, undo);
        editor().transactionManager().commit(transaction);
    }
}

void EntityInspector::inspectTransform() {
    using namespace propertiesView;
    auto& transform = entity.get<Transform>();
    if (!beginComponentSection("Transform")) {
        return;
    }
    auto& style = ImGui::GetStyle();
    beginProperty("Position");
    float2 lockButtonSize = ImGui::GetFontSize() + style.FramePadding.y * 2;
    float width = ImGui::GetContentRegionAvail().x - lockButtonSize.x -
                  style.ItemSpacing.x;
    ImGui::SetNextItemWidth(width);
    editComponent(transform, [&](TAToken&) {
        dragFloat3Labelled("###Position", transform.position.data());
    });
    beginProperty("Orientation");
    ImGui::SetNextItemWidth(width);
    editComponent(transform, [&](TAToken&) {
        double const radToDegrees = 180 / vml::constants<>::pi;
        float3 euler = vml::to_euler(transform.orientation) * radToDegrees;
        if (dragFloat3Labelled("###Orientation", euler.data(),
                               { .format = "%.3f°" }))
        {
            transform.orientation = vml::to_quaternion(euler / radToDegrees);
        }
    });
    beginProperty("Scale");
    ImGui::SetNextItemWidth(width);
    editComponent(transform, [&](TAToken&) {
        dragFloat3Labelled("###Scale", transform.scale.data(),
                           { .speed = 0.02,
                             .linkFields = transformScaleLinked });
    });
    ImGui::SameLine();
    linkButton(transformScaleLinked, lockButtonSize);
    endSection();
}

void EntityInspector::inspectMesh() {
    using namespace propertiesView;
    if (!beginComponentSection<MeshRendererComponent>("Mesh Renderer")) {
        return;
    }
    auto& meshRenderer = entity.get<MeshRendererComponent>();
    using enum AssetRepresentation;
    beginProperty("Mesh");
    editComponent(meshRenderer, [&](TAToken& token) {
        fullWidth();
        if (assetButton("###Mesh", meshRenderer.mesh, { .repr = GPU })) {
            token.commit();
        }
    });
    beginProperty("Material");
    editComponent(meshRenderer, [&](TAToken& token) {
        fullWidth();
        if (assetButton("###MaterialInstance", meshRenderer.materialInstance,
                        { .repr = GPU }))
        {
            token.commit();
        }
    });
    endSection();
}

template <typename E>
static std::optional<E> myEnumCombo(char const* label, E value, auto&& values) {
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
        for (auto current: values) {
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
static std::optional<E> myEnumCombo(char const* label, E value) {
    return myEnumCombo(label, value, magic_enum::enum_values<E>());
}

template <typename E>
static std::optional<E> inspectTypeGeneric(char const* ID, E type,
                                           auto&& values, auto exchange) {
    auto newType = myEnumCombo(ID, type, values);
    if (newType) {
        std::invoke(exchange, *newType);
    }
    return newType;
}

template <typename E>
static std::optional<E> inspectTypeGeneric(char const* ID, E type,
                                           std::initializer_list<E> values,
                                           auto exchange) {
    return inspectTypeGeneric(ID, type, std::span(values), exchange);
}

template <typename E>
static std::optional<E> inspectTypeGeneric(char const* ID, E type,
                                           auto exchange) {
    return inspectTypeGeneric(ID, type, magic_enum::enum_values<E>(), exchange);
}

static Transaction* makeComponentExchange(EntityHandle entity,
                                          ComponentKind oldType,
                                          ComponentKind newType) {
    return visitComponentType(oldType, [&]<typename Old>(utl::tag<Old>) {
        auto* ptr = entity.tryGet<Old>();
        std::optional<Old> old = ptr ? std::optional(*ptr) : std::nullopt;
        auto commit = [=](EntityHandle entity) {
            if (old) {
                entity.tryRemove<Old>();
                entity.tryAdd(newType);
                return true;
            }
            return false;
        };
        auto undo = [=](EntityHandle entity) {
            if (old) {
                entity.tryAdd(*old);
                entity.tryRemove(newType);
                return true;
            }
            return false;
        };
        return new CustomEntityTransaction(entity.strongHandle(), commit, undo);
    });
}

void EntityInspector::inspectLight() {
    using namespace propertiesView;
    auto type = getLightType2(entity).value();
    auto deleter = [&] { entity.remove(type); };
    auto font = FontDesc::UIDefault().setWeight(FontWeight::Semibold);
    if (!beginGenericSection("Light Component", font, deleter)) {
        return;
    }
    beginProperty("Type");
    fullWidth();
    type = inspectTypeGeneric("###Type", type, [&](ComponentKind newType) {
        auto color = visitComponentType(type, [&]<typename C>(utl::tag<C>) {
            if constexpr (requires { entity.get<C>().color; }) {
                return entity.get<C>().color;
            }
            else {
                return packed_float3{};
            }
        });
        makeComponentExchange(entity, type, newType);
#if 0
        dispatchComponentType(newType, [&]<typename C>(utl::tag<C>) {
            auto& c = entity.add(C{ C::Light::Default() });
            c.color = color;
        });
        removeComponentType(type, entity);
#endif
    }).value_or(type);
    visitComponentType<PointLightComponent, SpotLightComponent,
                       DirectionalLightComponent,
                       SkyLightComponent>(type, [&]<typename L>(utl::tag<L>) {
        inspectLightComponent(entity.get<L>());
    });
    endSection();
}

void EntityInspector::inspectLightCommon(ComponentType auto& light,
                                         LightType type) {
    using namespace propertiesView;
    beginProperty("Color");
    editComponent(light, [&](TAToken&) {
        auto flags = ImGuiColorEditFlags_NoInputs |
                     ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_Float |
                     ImGuiColorEditFlags_NoAlpha |
                     ImGuiColorEditFlags_PickerHueWheel;
        ImGui::ColorEdit3("###Color", light.color.data(), flags);
    });
    beginProperty("Intensity");
    float speed = type == LightType::DirectionalLight ? 0.01 :
                  type == LightType::SkyLight         ? 0.0001 :
                                                        100;
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::DragFloat("###Intensity", &light.intensity, speed, 0, FLT_MAX,
                         "%f");
    });
}

void EntityInspector::inspectLightComponent(PointLightComponent& light) {
    using namespace propertiesView;
    inspectLightCommon(light, LightType::PointLight);
    beginProperty("Radius");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###Radius", &light.radius, 0, 100);
    });
    beginProperty("Falloff");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::DragFloat("###Falloff", &light.falloff,
                         /* speed: */ 0.01, /* min: */ 0.01,
                         /* max: */ FLT_MAX);
    });
}

void EntityInspector::inspectLightComponent(SpotLightComponent& light) {
    using namespace propertiesView;
    inspectLightCommon(light, LightType::SpotLight);
    beginProperty("Radius");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###Radius", &light.radius, 0, 100);
    });
    float inner = light.innerCutoff;
    float outer = light.outerCutoff;
    float angle = (inner + outer) / 2;
    float falloff = (outer - inner) / 2;
    beginProperty("Angle");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###Angle", &angle, 0, 1);
    });
    beginProperty("Falloff");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###Falloff", &falloff, 0, 0.2);
    });
    light.innerCutoff = angle - falloff;
    light.outerCutoff = angle + falloff;
}

void EntityInspector::inspectLightComponent(DirectionalLightComponent& light) {
    using namespace propertiesView;
    inspectLightCommon(light, LightType::DirectionalLight);
    beginProperty("Source Size");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::DragFloat("###SourceSize", &light.sourceSize, 0.01, 0, FLT_MAX);
    });
    beginProperty("Casts Shadow");
    editComponent(light, [&](TAToken&) {
        ImGui::Checkbox("###CastsShadow", &light.castsShadows);
    });
    if (!light.castsShadows) {
        return;
    }
    beginProperty("Shadow Distance");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::DragFloat("###ShadowDistance", &light.shadowDistance, 1, 0,
                         FLT_MAX);
    });
    beginProperty("Shadow Z Distance");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::DragFloat("###ShadowDistanceZ", &light.shadowDistanceZ, 1, 0,
                         FLT_MAX);
    });
    beginProperty("Number Of Cascades");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        int nc = light.numCascades;
        ImGui::SliderInt("###NumCascades", &nc, 1, 10);
        light.numCascades = nc;
    });
    beginProperty("Distribution Exponent");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###CascadeDistributionExponent",
                           &light.cascadeDistributionExponent, 1, 4);
    });
    beginProperty("Transition");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###CascadeTransitionFraction",
                           &light.cascadeTransitionFraction, 0, 1);
    });
    beginProperty("Fadeout");
    fullWidth();
    editComponent(light, [&](TAToken&) {
        ImGui::SliderFloat("###ShadowDistanceFadeoutFraction",
                           &light.shadowDistanceFadeoutFraction, 0, 1);
    });
}

void EntityInspector::inspectLightComponent(SkyLightComponent& light) {
    inspectLightCommon(light, LightType::SkyLight);
    using namespace propertiesView;
    beginProperty("IBL Texture");
    fullWidth();
    using enum AssetRepresentation;
    editComponent(light, [&](TAToken&) {
        assetButton("###IBLTexture", light.cubemap, { .repr = GPU | CPU });
    });
}

void EntityInspector::inspectPhysics() {
    using namespace propertiesView;
    auto physicsType = getPhysicsType2(entity).value();
    auto colliderType = getColliderType2(entity).value();
    auto deleter = [&] {
        entity.remove(physicsType);
        entity.remove(colliderType);
    };
    auto font = FontDesc::UIDefault().setWeight(FontWeight::Semibold);
    if (!beginGenericSection("Physics", font, deleter)) {
        return;
    }
    beginProperty("Movability");
    fullWidth();
    physicsType = inspectTypeGeneric("###Movability", physicsType,
                                     { ComponentKind::PhysicsStaticComponent,
                                       ComponentKind::PhysicsMovableComponent },
                                     [&](ComponentKind newType) {
        auto* transaction = makeComponentExchange(entity, physicsType, newType);
        editor().transactionManager().commit(transaction);
    }).value_or(physicsType);
    beginProperty("Collider");
    fullWidth();
    colliderType =
        inspectTypeGeneric("###Collider", colliderType,
                           { ComponentKind::BoxColliderComponent,
                             ComponentKind::SphereColliderComponent },
                           [&](ComponentKind newType) {
        auto* transaction =
            makeComponentExchange(entity, colliderType, newType);
        editor().transactionManager().commit(transaction);
    }).value_or(colliderType);
    visitComponentType<SphereColliderComponent,
                       BoxColliderComponent>(colliderType,
                                             [&]<typename C>(utl::tag<C>) {
        inspectColliderComponent(entity.get<C>());
    });
    endSection();
}

void EntityInspector::inspectColliderComponent(
    SphereColliderComponent& collider) {
    using namespace propertiesView;
    beginProperty("Radius");
    fullWidth();
    editComponent(collider, [&](TAToken&) {
        ImGui::DragFloat("###Radius", &collider.radius);
    });
}

void EntityInspector::inspectColliderComponent(BoxColliderComponent& collider) {
    using namespace propertiesView;
    beginProperty("Half Extend");
    fullWidth();
    editComponent(collider, [&](TAToken&) {
        dragFloat3Labelled("###HalfExtend", collider.halfExtend.data(),
                           { .showLinkButton = true,
                             .labelStyle = DragFloat3Options::Slim });
    });
}

void EntityInspector::inspectScript() {
    using namespace propertiesView;
    using namespace scatha;
    if (!entity.has<ScriptPreservedData>()) {
        ImGui::Text("Error: No preserved data");
        return;
    }
    auto& scriptSystem = editor().scriptSystem();
    auto* sym = scriptSystem.symbolTable();
    auto& component = entity.get<ScriptComponent>();
    auto& preservedData = entity.get<ScriptPreservedData>();
    if (!sym) {
        ImGui::Text("Error: No symbol table");
        return;
    }
    if (!beginComponentSection<ScriptComponent>("Script Component")) {
        return;
    }
    beginProperty("Class");
    fullWidth();
    char const* name = component.type ? component.type->name().data() : "";
    if (ImGui::BeginCombo("###Class", name)) {
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
    sema::StructType const* type = component.type;
    bool haveObject = component.objectAddress != 0;
    if (!type) {
        endSection();
        return;
    }
    header("Attributes");
    ImGui::BeginDisabled(!haveObject);
    for (auto* variable: type->memberVariables()) {
        if (!variable->isPublic()) {
            continue;
        }
        beginProperty(variable->name());
        SC_MATCH(*variable->type()){ [&](sema::FloatType const& type) {
            fullWidth();
            float value = 0;
            ImGui::DragFloat(std::string(type.name()).c_str(), &value);
        }, [&](sema::Type const&) {} };
    }
    ImGui::EndDisabled();

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
    endSection();
}

/// MARK: - Helpers
bool EntityInspector::beginComponentSection(std::string_view name) {
    return beginComponentSection<void>(name);
}
template <typename T>
bool EntityInspector::beginComponentSection(std::string_view name) {
    if constexpr (std::is_same_v<T, void>) {
        return beginGenericSection(name,
                                   FontDesc::UIDefault(FontWeight::Semibold),
                                   nullptr);
    }
    else {
        return beginGenericSection(name,
                                   FontDesc::UIDefault(FontWeight::Semibold),
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
    if (!deleter) {
        return beginSection();
    }
    // 'Delete' Button
    ImGui::PushFont(FontManager::get(FontWeight::Semibold));
    bool open = true;
    float2 textSize = ImGui::CalcTextSize("Delete");
    float2 buttonPos = {
        cursorPos.x + ImGui::GetContentRegionAvail().x - textSize.x, cursorPos.y
    };
    ImGui::SetCursorPos(buttonPos);
    char deleteButtonLabel[64] = "##delete-";
    std::strncpy(&deleteButtonLabel[9], name.data(), 64 - 9 - 1);
    if (ImGui::InvisibleButton(deleteButtonLabel, textSize)) {
        deleter();
        open = false;
    }
    float4 color = GImGui->Style.Colors[ImGuiCol_TextDisabled];
    float4 hightlightColor =
        (float4(GImGui->Style.Colors[ImGuiCol_Text]) + color) / 2;
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::IsItemHovered() ? hightlightColor : color);
    ImGui::SetCursorPos(buttonPos);
    ImGui::Text("Delete");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    return open && beginSection();
}

void EntityInspector::endSection() { propertiesView::endSection(); }

void EntityInspector::endSubSection() {
    endSection();
    propertiesView::beginSection();
}
