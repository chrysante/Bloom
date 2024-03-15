#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/Editor/Views/EntityInspector.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <mtl/mtl.hpp>
// #include <scatha/Sema/Entity.h>
// #include <scatha/Sema/SymbolTable.h>
#include <utl/typeinfo.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Runtime/ScriptSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/SelectionContext.h"
#include "Poppy/Editor/Views/AssetBrowser.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace mtl::short_types;
using namespace bloom;
using namespace poppy;

POPPY_REGISTER_VIEW(EntityInspector, "Entity Inspector", {});

EntityInspector::EntityInspector(): BasicSceneInspector(this) {}

void EntityInspector::frame() {
    if (scenes().empty()) {
        displayEmptyWithReason("No active Scene");
        return;
    }
    bloom::EntityHandle const entity = selection().empty() ?
                                           bloom::EntityHandle{} :
                                           *selection().entities().begin();
    if (!entity) {
        displayEmptyWithReason("No Entity selected");
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
    if (entity.has<ScriptComponent>()) {
        inspectScript(entity);
        ImGui::Separator();
    }
}

void EntityInspector::inspectTag(bloom::EntityHandle entity) {
    using namespace bloom;
    auto& tag = entity.get<TagComponent>();
    std::string_view const name = tag.name;

    float2 const framePadding = GImGui->Style.FramePadding;
    ImGui::BeginChild("##inspect-tag-child",
                      { 0, GImGui->FontSize + 2 * framePadding.y });

    float2 const spacing = GImGui->Style.ItemSpacing;

    withFont(FontWeight::bold, FontStyle::roman, [&] {
        float2 const windowSize = ImGui::GetWindowSize();
        float2 const addButtonSize =
            float2(ImGui::CalcTextSize("Add Component")) +
            float2(4, 2) * framePadding;

        float2 const nameTextSize = {
            windowSize.x - addButtonSize.x - spacing.x, windowSize.y
        };

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
                                  appearance.style()
                                      .colors.highlightControlFrame);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                                  appearance.style()
                                      .colors.highlightControlFrameHovered);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                float2(2, 1) * framePadding);
            ImGui::SetNextItemWidth(addButtonSize.x);
            bool const comboOpen =
                ImGui::BeginCombo("##-add-component", "Add Component",
                                  ImGuiComboFlags_NoArrowButton);
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();

            if (comboOpen) {
                withFont(Font::UIDefault(), [&] {
                    auto addComponentButton =
                        [&]<typename Component>(utl::tag<Component>,
                                                char const* name,
                                                EntityHandle entity,
                                                bool forceDisable = false) {
                        auto flags = 0;
                        if (entity.has<Component>() || forceDisable) {
                            flags |= ImGuiSelectableFlags_Disabled;
                        }
                        if (ImGui::Selectable(name, false, flags)) {
                            entity.add(Component{});
                        }
                    };
                    bool const hasLight = hasLightComponent(entity);
                    addComponentButton(utl::tag<MeshRendererComponent>{},
                                       "Mesh Renderer", entity);
                    addComponentButton(utl::tag<PointLightComponent>{},
                                       "Point Light", entity, hasLight);
                    addComponentButton(utl::tag<SpotLightComponent>{},
                                       "Spot Light", entity, hasLight);
                    addComponentButton(utl::tag<DirectionalLightComponent>{},
                                       "Directional Light", entity, hasLight);
                    addComponentButton(utl::tag<SkyLightComponent>{},
                                       "Sky Light", entity, hasLight);
                    addComponentButton(utl::tag<ScriptComponent>{}, "Script",
                                       entity);
                });
                ImGui::EndCombo();
            }
        });
    });

    ImGui::EndChild();
}

void EntityInspector::inspectTransform(bloom::EntityHandle entity) {
    using namespace propertiesView;
    auto& transform = entity.get<Transform>();

    if (beginComponentSection("Transform")) {
        disabledIf(isSimulating(), [&] {
            auto& style = ImGui::GetStyle();
            beginProperty("Position");

            float2 const lockButtonSize =
                ImGui::CalcTextSize("").y + style.FramePadding.y * 2;

            float const width = ImGui::GetContentRegionAvail().x -
                                lockButtonSize.x - style.ItemSpacing.x;

            ImGui::SetNextItemWidth(width);
            dragFloat3Pretty("##position", transform.position.data());

            beginProperty("Orientation");
            float3 euler = mtl::to_euler(transform.orientation) * 180;

            ImGui::SetNextItemWidth(width);
            if (dragFloat3Pretty("##orientation", euler.data())) {
                transform.orientation = mtl::to_quaternion(euler / 180);
            }

            beginProperty("Scale");
            ImGui::SetNextItemWidth(width);
            auto const oldScale = transform.scale;
            if (dragFloat3Pretty("##scale", transform.scale.data(), 0.02) &&
                transformScaleLinked)
            {
                bool3 const edited =
                    mtl::map(transform.scale, oldScale, utl::unequals);
                if (edited.x) {
                    transform.scale = transform.scale.x;
                }
                else if (edited.y) {
                    transform.scale = transform.scale.y;
                }
                else {
                    transform.scale = transform.scale.z;
                }
            }
            ImGui::SameLine();
            auto* const iconFont = icons.font(IconSize::_16);
            auto const unicode =
                icons.unicodeStr(transformScaleLinked ? "link" : "unlink");
            ImGui::PushFont(iconFont);
            bool const tsLinked = transformScaleLinked;
            if (tsLinked) {
                ImGui::PushStyleColor(ImGuiCol_Button,
                                      appearance.style()
                                          .colors.highlightControlFrame);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                      appearance.style()
                                          .colors.highlightControlFrameHovered);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                      appearance.style()
                                          .colors.highlightControlFrameDown);
            }
            if (ImGui::Button(unicode.data(), lockButtonSize)) {
                transformScaleLinked ^= true;
            }
            if (tsLinked) {
                ImGui::PopStyleColor(3);
            }
            ImGui::PopFont();
        });
        endSection();
    }
}

void EntityInspector::inspectMesh(bloom::EntityHandle entity) {
    using namespace propertiesView;
    auto& meshRenderer = entity.get<MeshRendererComponent>();

    if (beginComponentSection<MeshRendererComponent>("Mesh Renderer", entity)) {
        disabledIf(isSimulating(), [&] {
            beginProperty("Mesh");
            ImGui::Button(meshRenderer.mesh ?
                              assetManager()
                                  .getName(meshRenderer.mesh->handle())
                                  .data() :
                              "No mesh assigned",
                          { ImGui::GetContentRegionAvail().x, 0 });
            receiveMeshDragDrop(entity);

            beginProperty("Material");
            ImGui::Button(meshRenderer.materialInstance ?
                              assetManager()
                                  .getName(
                                      meshRenderer.materialInstance->handle())
                                  .data() :
                              "No material assigned",
                          { ImGui::GetContentRegionAvail().x, 0 });
            receiveMaterialDragDrop(entity);
        });
        endSection();
    }
}

void EntityInspector::receiveMeshDragDrop(bloom::EntityHandle entity) {
    using namespace bloom;
    auto const payload = acceptAssetDragDrop({ AssetType::StaticMesh });
    if (!payload) {
        return;
    }
    auto const handle = *payload;
    Logger::Trace("Received Asset: ", assetManager().getName(handle));

    auto asset = as<StaticMesh>(assetManager().get(handle));
    assert(!!asset);

    assetManager().makeAvailable(handle, AssetRepresentation::GPU);

    auto& meshRenderer = entity.get<MeshRendererComponent>();
    meshRenderer.mesh = std::move(asset);
}

void EntityInspector::receiveMaterialDragDrop(bloom::EntityHandle entity) {
    using namespace bloom;
    auto const payload = acceptAssetDragDrop({ AssetType::MaterialInstance });
    if (!payload) {
        return;
    }
    auto const handle = *payload;
    Logger::Trace("Received Asset: ", assetManager().getName(handle));

    auto materialInstance = as<MaterialInstance>(assetManager().get(handle));
    assert((bool)materialInstance);

    assetManager().makeAvailable(handle, AssetRepresentation::CPU |
                                             AssetRepresentation::GPU);

    auto& meshRenderer = entity.get<MeshRendererComponent>();
    meshRenderer.materialInstance = std::move(materialInstance);
}

void EntityInspector::inspectLight(bloom::EntityHandle entity) {
    using namespace bloom;
    auto type = getLightType(entity);

    auto deleter = [&] {
        switch (type) {
        case LightType::pointlight:
            entity.remove<PointLightComponent>();
            break;
        case LightType::spotlight:
            entity.remove<SpotLightComponent>();
            break;
        case LightType::directional:
            entity.remove<DirectionalLightComponent>();
            break;

        default:
            BL_DEBUGBREAK();
            break;
        }
    };

    if (beginGenericSection("Light Component",
                            Font::UIDefault().setWeight(FontWeight::semibold),
                            deleter))
    {
        disabledIf(isSimulating(), [&] {
            inspectLightType(type, entity);

            switch (type) {
            case LightType::pointlight:
                inspectPointLight(entity.get<PointLightComponent>().light);
                break;
            case LightType::spotlight:
                inspectSpotLight(entity.get<SpotLightComponent>().light);
                break;
            case LightType::directional:
                inspectDirectionalLight(
                    entity.get<DirectionalLightComponent>().light);
                break;
            case LightType::skylight:
                inspectSkyLight(entity.get<SkyLightComponent>().light);
                break;

            default:
                break;
            }
        });
        endSection();
    }
}

void EntityInspector::inspectLightType(LightType& type,
                                       bloom::EntityHandle entity) {
    using namespace propertiesView;
    beginProperty("Type");
    auto newType = type;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##Light Type", toString(newType).data())) {
        for (size_t j = 0; j < (size_t)LightType::_count; ++j) {
            LightType const i = (LightType)j;
            bool const selected = newType == i;
            if (ImGui::Selectable(toString(i).data(), selected)) {
                newType = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (newType != type) {
        auto const common = getLightCommon(type, entity);
        removeLightComponent(type, entity);
        dispatchLightComponent(newType, [&]<typename T>(utl::tag<T>) {
            entity.add(T{ common });
        });
    }
    type = newType;
}

void EntityInspector::inspectLightCommon(bloom::LightCommon& light,
                                         LightType type) {
    using namespace propertiesView;
    beginProperty("Color");
    ImGui::ColorEdit3("##light-color", light.color.data(),
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_Float |
                          ImGuiColorEditFlags_NoAlpha |
                          ImGuiColorEditFlags_PickerHueWheel);
    beginProperty("Intensity");
    float const speed = type == LightType::directional ? 0.01 :
                        type == LightType::skylight    ? 0.0001 :
                                                         100;
    fullWidth();
    ImGui::DragFloat("intensity", &light.intensity, speed, 0, FLT_MAX, "%f");
}

void EntityInspector::inspectPointLight(bloom::PointLight& light) {
    using namespace propertiesView;
    inspectLightCommon(light.common, LightType::pointlight);

    beginProperty("Radius");
    fullWidth();
    ImGui::SliderFloat("radius", &light.radius, 0, 100);
}

void EntityInspector::inspectSpotLight(bloom::SpotLight& light) {
    using namespace propertiesView;
    inspectLightCommon(light.common, LightType::spotlight);

    beginProperty("Radius");
    fullWidth();
    ImGui::SliderFloat("radius", &light.radius, 0, 100);

    float const inner = light.innerCutoff;
    float const outer = light.outerCutoff;

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

void EntityInspector::inspectDirectionalLight(bloom::DirectionalLight& light) {
    using namespace propertiesView;
    inspectLightCommon(light.common, LightType::directional);

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

    beginProperty("Cascade Distribution Exponent");
    fullWidth();
    ImGui::SliderFloat("cascade-distribution-exponent",
                       &light.cascadeDistributionExponent, 1, 4);

    beginProperty("Cascade Transition Fraction");
    fullWidth();
    ImGui::SliderFloat("cascade-transition-fraction",
                       &light.cascadeTransitionFraction, 0, 1);

    beginProperty("Distance Fadeout Fraction");
    fullWidth();
    ImGui::SliderFloat("shadow-distance-fadeout-fraction",
                       &light.shadowDistanceFadeoutFraction, 0, 1);
}

void EntityInspector::inspectSkyLight(bloom::SkyLight& light) {
    inspectLightCommon(light.common, LightType::skylight);
}

template <typename T>
static void editDataField(std::string_view id, T*) {
    ImGui::Text("No Impl for %s", utl::nameof<T>.data());
}

template <>
void editDataField(std::string_view id, float* value) {
    ImGui::PushID(generateUniqueID(id, 0).data());

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::DragFloat(generateUniqueID(id, 1).data(), value);

    ImGui::PopID();
}

void EntityInspector::inspectScript(bloom::EntityHandle entity) {
    using namespace propertiesView;
#if 0
    auto& scriptSystem = editor().coreSystems().scriptSystem();
    auto* program = assetManager().getProgram();
    if (!program) {
        return;
    }
    auto& symTable = program->symbolTable();
    auto& script = entity.get<ScriptComponent>();

    if (beginComponentSection<ScriptComponent>("Script", entity)) {
        disabledIf(isSimulating(), [&] {
            beginProperty("Class");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            auto name = script.classType ? script.classType->name().data() : "";
            if (ImGui::BeginCombo("##-script", name)) {
                for (auto* classType: symTable.structDependencyOrder()) {
                    bool const selected = classType == script.classType;
                    if (ImGui::Selectable(classType->name().data(), selected)) {
                        script.classType = classType;
                        script.object =
                            scriptSystem.instantiateObject(classType);
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (script.object && beginSubSection("Attributes")) {
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
            }
        });
        endSection();
    }
#endif
}

/// MARK: - Helpers
bool EntityInspector::beginComponentSection(std::string_view name) {
    return beginComponentSection<void>(name, EntityHandle{});
}
template <typename T>
bool EntityInspector::beginComponentSection(std::string_view name,
                                            bloom::EntityHandle entity) {
    if constexpr (std::is_same_v<T, void>) {
        return beginGenericSection(name,
                                   Font::UIDefault().setWeight(
                                       FontWeight::semibold),
                                   nullptr);
    }
    else {
        return beginGenericSection(name,
                                   Font::UIDefault().setWeight(
                                       FontWeight::semibold),
                                   [&] { entity.remove<T>(); });
    }
}

bool EntityInspector::beginSubSection(std::string_view name) {
    endSection();
    return beginGenericSection(name,
                               Font::UIDefault().setStyle(FontStyle::italic),
                               nullptr);
}

bool EntityInspector::beginGenericSection(std::string_view name,
                                          Font const& font,
                                          utl::function<void()> deleter) {
    using namespace propertiesView;
    float2 const cursorPos = ImGui::GetCursorPos();
    header(name, font);
    bool const open = [&] {
        if (!deleter) {
            return true;
        }

        // 'Delete' Button
        return withFont(FontWeight::semibold, FontStyle::roman, [&] {
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
