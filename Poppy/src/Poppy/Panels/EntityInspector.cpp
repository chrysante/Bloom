#define IMGUI_DEFINE_MATH_OPERATORS

#include "EntityInspector.hpp"

#include "AssetBrowser.hpp"

#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/SelectionContext.hpp"
#include "Poppy/Debug.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>

using namespace mtl::short_types;
using namespace bloom;

namespace poppy {
	
	EntityInspector::EntityInspector():
		Panel("Entity Inspector")
	{
		
	}
	
	void EntityInspector::display() {
		if (!scene()) {
			return;
		}
		bloom::EntityID const activeEntity = selection()->empty() ? bloom::EntityID{} : selection()->ids()[0];
		auto const id = activeEntity;
		
		if (!id) {
			return;
		}
		
		auto const entity = scene()->getHandle(id);
		
		if (entity.has<TagComponent>()) {
			inspectTag(entity);
			ImGui::Separator();
		}
		
		if (entity.has<TransformComponent>()) {
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
	
	void EntityInspector::inspectTag(bloom::EntityID entity) {
		using namespace bloom;
		auto& tag = scene()->getComponent<TagComponent>(entity);
		std::string_view const name = tag.name;
		
		float2 const framePadding = GImGui->Style.FramePadding;
		ImGui::BeginChild("##inspect-tag-child", { 0, GImGui->FontSize + 2 * framePadding.y });
		
		float2 const spacing = GImGui->Style.ItemSpacing;
		
		withFont(FontWeight::bold, FontStyle::roman, [&]{
			float2 const windowSize = ImGui::GetWindowSize();
			float2 const addButtonSize = float2(ImGui::CalcTextSize("Add Component")) + 2 * framePadding;
			
			// name field
			float2 const nameTextSize = { windowSize.x - addButtonSize.x - spacing.x, windowSize.y };
			if (editingNameState) {
				ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
				
				char buffer[256]{};
				std::strncpy(buffer, name.data(), 255);
				ImGui::SetNextItemWidth(nameTextSize.x);
				if (editingNameState > 1) { ImGui::SetKeyboardFocusHere(); }
				if (ImGui::InputText("##name-input", buffer, 256)) {
					tag.name = buffer;
				}
				editingNameState = ImGui::IsWindowFocused();
				
				ImGui::PopStyleColor(1);
			}
			else {
				ImGui::SetCursorPos(framePadding);
				ImGui::SetNextItemWidth(nameTextSize.x);
				ImGui::Text("%s", name.data());
				ImGui::SetCursorPos({});
				editingNameState = ImGui::InvisibleButton("activate-name-input",
														  nameTextSize,
														  ImGuiButtonFlags_PressedOnDoubleClick) * 2;
			}
			
			// 'add' button
			ImGui::SetCursorPos({ nameTextSize.x + spacing.x, 0 });
			using Color = mtl::colors<float3>;
			
			auto convert = [](float3 x, float a) { return ImGui::ColorConvertFloat4ToU32(float4{ x, a }); };
			
			ImGui::PushStyleColor(ImGuiCol_FrameBg,        convert(Color::green, .35));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, convert(Color::green, .5));
		
			ImGui::SetNextItemWidth(addButtonSize.x);
			bool const comboOpen = ImGui::BeginCombo("##-add-component", "Add Component", ImGuiComboFlags_NoArrowButton);

			ImGui::PopStyleColor(2);
			
			
			if (comboOpen) {
				withFont(FontWeight::regular, FontStyle::roman, [&]{
					auto addComponentButton = [&]<typename Component>(utl::tag<Component>,
																	  char const* name,
																	  EntityID entity,
																	  bool forceDisable = false)
					{
						auto flags = 0;
						if (scene()->hasComponent<Component>(entity) || forceDisable) {
							flags |= ImGuiSelectableFlags_Disabled;
						}
						if (ImGui::Selectable(name, false, flags)) {
							scene()->addComponent(entity, Component{});
						}
					};
					bool const hasLight = hasLightComponent(entity);
					addComponentButton(utl::tag<MeshRendererComponent>{},     "Mesh Renderer",     entity);
					addComponentButton(utl::tag<PointLightComponent>{},       "Point Light",       entity, hasLight);
					addComponentButton(utl::tag<SpotLightComponent>{},        "Spot Light",        entity, hasLight);
					addComponentButton(utl::tag<DirectionalLightComponent>{}, "Directional Light", entity, hasLight);
					addComponentButton(utl::tag<SkyLightComponent>{},         "Sky Light",         entity, hasLight);
					addComponentButton(utl::tag<ScriptComponent>{},           "Script",            entity);
				});
				ImGui::EndCombo();
			}
		});
		
		ImGui::EndChild();
	}
	
	void EntityInspector::inspectTransform(bloom::EntityID entity) {
		using namespace bloom;
		auto& transform = scene()->getComponent<TransformComponent>(entity);
	
		header("Transform");
		if (beginSection()) {
			beginProperty("Position");
			dragFloat3Pretty(transform.position.data(), "-position");
			
			beginProperty("Orientation");
			float3 euler = mtl::to_euler(transform.orientation) * 180;
			if (dragFloat3Pretty(euler.data(), "-orientation")) {
				transform.orientation = mtl::to_quaternion(euler / 180);
			}
			
			beginProperty("Scale");
			dragFloat3Pretty(transform.scale.data(), "-scale", 0.02);

			endSection();
		}
	}
	
	void EntityInspector::inspectMesh(bloom::EntityID entity) {
		auto& meshRenderer = scene()->getComponent<MeshRendererComponent>(entity);
		
		componentHeader<MeshRendererComponent>("Mesh Renderer", entity);
		if (beginSection()) {
			beginProperty("Mesh");
			ImGui::Button(meshRenderer.mesh ?
						  assetManager()->getName(meshRenderer.mesh->handle()).data() :
						  "No mesh assigned",
						  { ImGui::GetContentRegionAvail().x, 0 });
			recieveMeshDragDrop(entity);
	
			beginProperty("Material");
			ImGui::Button(meshRenderer.material ?
						  assetManager()->getName(meshRenderer.material->handle()).data() :
						  "No material assigned",
						  { ImGui::GetContentRegionAvail().x, 0 });
			recieveMaterialDragDrop(entity);
			
			endSection();
		}
	}
	
	void EntityInspector::recieveMeshDragDrop(bloom::EntityID entity) {
		using namespace bloom;
		auto const payload = acceptAssetDragDrop(AssetType::staticMesh);
		if (!payload) {
			return;
		}
		auto const handle = *payload;
		poppyLog(trace, "Recieved Asset: {}", assetManager()->getName(handle));
		
		auto asset = as<StaticMeshAsset>(assetManager()->get(handle));
		poppyAssert(!!asset);
		
		assetManager()->makeAvailable(handle, AssetRepresentation::GPU);
			
		auto& meshRenderer = scene()->getComponent<MeshRendererComponent>(entity);
		meshRenderer.mesh = std::move(asset);
	}
	
	void EntityInspector::recieveMaterialDragDrop(bloom::EntityID entity) {
		using namespace bloom;
		auto const payload = acceptAssetDragDrop(AssetType::material);
		if (!payload) {
			return;
		}
		auto const handle = *payload;
		poppyLog(trace, "Recieved Asset: {}", assetManager()->getName(handle));
		
		auto asset = as<MaterialAsset>(assetManager()->get(handle));
		poppyAssert(!!asset);
		
		assetManager()->makeAvailable(handle, AssetRepresentation::GPU);
			
		auto& meshRenderer = scene()->getComponent<MeshRendererComponent>(entity);
		meshRenderer.material = std::move(asset);
	}
	
	void EntityInspector::inspectLight(bloom::EntityID entity) {
		using namespace bloom;
		auto type = getLightType(entity);
		
		auto deleter = [&]{
			switch (type) {
				case LightType::pointlight:
					scene()->removeComponent<PointLightComponent>(entity);
					break;
				case LightType::spotlight:
					scene()->removeComponent<SpotLightComponent>(entity);
					break;
				case LightType::directional:
					scene()->removeComponent<DirectionalLightComponent>(entity);
					break;
					
				default:
					poppyDebugbreak();
					break;
			}
		};
		
		if (componentHeaderEx("Light Component", deleter) && beginSection()) {
			inspectLightType(type, entity);
	
			switch (type) {
				case LightType::pointlight:
					inspectPointLight(scene()->getComponent<PointLightComponent>(entity).light);
					break;
				case LightType::spotlight:
					inspectSpotLight(scene()->getComponent<SpotLightComponent>(entity).light);
					break;
				case LightType::directional:
					inspectDirectionalLight(scene()->getComponent<DirectionalLightComponent>(entity).light);
					break;
				case LightType::skylight:
					inspectSkyLight(scene()->getComponent<SkyLightComponent>(entity).light);
					break;
					
				default:
					break;
			}
			
			endSection();
		}
	}
	
	void EntityInspector::inspectLightType(LightType& type, bloom::EntityID entity) {
		using namespace bloom;
		beginProperty("Type");
		auto newType = type;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##Light Type",
							  toString(newType).data()))
		{
			for (auto i: utl::enumerate<LightType>()) {
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
			switch (newType) {
				case LightType::pointlight:
					scene()->addComponent(entity, PointLightComponent{ common });
					break;
					
				case LightType::spotlight:
					scene()->addComponent(entity, SpotLightComponent{ common });
					break;
					
				case LightType::directional:
					scene()->addComponent(entity, DirectionalLightComponent{ common });
					break;
				
				case LightType::skylight:
					scene()->addComponent(entity, SkyLightComponent{ common });
					break;
					
				default:
					poppyLog(error, "Other Light Types not implemented");
					break;
			}
		}
		type = newType;
	}
	
	void EntityInspector::inspectLightCommon(bloom::LightCommon& light, LightType type) {
		beginProperty("Color");
		ImGui::ColorEdit4("##light-color", light.color.data(),
						  ImGuiColorEditFlags_NoInputs |
						  ImGuiColorEditFlags_NoLabel |
						  ImGuiColorEditFlags_HDR);
		beginProperty("Intensity");
		float const speed =
			type == LightType::directional ?
			0.01 :
			type == LightType::skylight ?
			0.0001 :
			100;
		dragFloat("intensity", &light.intensity, speed, 0, FLT_MAX, "%f");
	}
	
	void EntityInspector::inspectPointLight(bloom::PointLight& light) {
		inspectLightCommon(light.common, LightType::pointlight);
		
		beginProperty("Radius");
		sliderFloat("radius", &light.radius, 0, 100);
	}
	
	void EntityInspector::inspectSpotLight(bloom::SpotLight& light) {
		inspectLightCommon(light.common, LightType::spotlight);
		
		beginProperty("Radius");
		sliderFloat("radius", &light.radius, 0, 100);
		
		float const inner = light.innerCutoff;
		float const outer = light.outerCutoff;
		
		float angle = (inner + outer) / 2;
		float falloff = (outer - inner) / 2;
		
		beginProperty("Angle");
		sliderFloat("angle", &angle, 0, 1);
		beginProperty("Falloff");
		sliderFloat("falloff", &falloff, 0, 0.2);
		
		light.innerCutoff = angle - falloff;
		light.outerCutoff = angle + falloff;
	}
	
	void EntityInspector::inspectDirectionalLight(bloom::DirectionalLight& light) {
		inspectLightCommon(light.common, LightType::directional);
		
		beginProperty("Casts Shadow");
		ImGui::Checkbox("##-casts-shadow", &light.castsShadows);
		
		if (!light.castsShadows) {
			return;
		}
		
		beginProperty("Shadow Distance");
		dragFloat("shadow-distance", &light.shadowDistance, 1, 0, FLT_MAX);
		
		beginProperty("Shadow Z Distance");
		dragFloat("shadow-distance-z", &light.shadowDistanceZ, 1, 0, FLT_MAX);
		
		beginProperty("Number Of Cascades");
		int nc = light.numCascades;
		sliderInt("num-cascades", &nc, 1, 10);
		light.numCascades = nc;
		
		beginProperty("Cascade Distribution Exponent");
		sliderFloat("cascade-distribution-exponent", &light.cascadeDistributionExponent, 1, 4);
		
		beginProperty("Cascade Transition Fraction");
		sliderFloat("cascade-transition-fraction", &light.cascadeTransitionFraction, 0, 1);
		
		beginProperty("Distance Fadeout Fraction");
		sliderFloat("shadow-distance-fadeout-fraction", &light.shadowDistanceFadeoutFraction, 0, 1);
	}
	
	void EntityInspector::inspectSkyLight(bloom::SkyLight& light) {
		inspectLightCommon(light.common, LightType::skylight);
	}
	
	void EntityInspector::inspectScript(bloom::EntityID id) {
		auto entity = scene()->getHandle(id);
		auto& script = entity.get<ScriptComponent>();
		
		componentHeader<ScriptComponent>("Script", id);
		if (beginSection()) {
			beginProperty("Class");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::BeginCombo("##-script", script.className.data())) {
				for (auto className: assetManager()->scriptClasses()) {
					bool const selected = script.className == className;
					if (ImGui::Selectable(className.data(), selected)) {
						script.className = className;
						script.object = getApplication().scriptEngine().instanciateObject(className);
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			endSection();
		}
	}
	
	/// MARK: - Helpers
	template <typename T>
	bool EntityInspector::componentHeader(std::string_view name, bloom::EntityID entity) {
		if constexpr (std::is_same_v<T, void>) {
			return componentHeaderEx(name, nullptr);
		}
		else {
			return componentHeaderEx(name, [&]{
				scene()->removeComponent<T>(entity);
			});
		}
	}
	
	bool EntityInspector::componentHeaderEx(std::string_view name,
											utl::function<void()> deleter)
	{
		float2 const cursorPos = ImGui::GetCursorPos();
		header(name);
		
		// 'Delete' Button
		return withFont(FontWeight::semibold, FontStyle::roman, [&]{
			bool result = true;
			if (deleter) {
				float2 const textSize = ImGui::CalcTextSize("Delete");
				float2 const buttonPos = { cursorPos.x + ImGui::GetContentRegionAvail().x - textSize.x, cursorPos.y };
				
				ImGui::SetCursorPos(buttonPos);
				char deleteButtonLabel[64] = "-delete-button";
				std::strncpy(&deleteButtonLabel[14], name.data(), 49);
				if (ImGui::InvisibleButton(deleteButtonLabel, textSize)) {
					deleter();
					result = false;
				}
				
				float4 const color = GImGui->Style.Colors[ImGuiCol_TextDisabled];
				float4 const hightlightColor = (float4(GImGui->Style.Colors[ImGuiCol_Text]) + color) / 2;
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::IsItemHovered() ? hightlightColor : color);
				ImGui::SetCursorPos(buttonPos);
				ImGui::Text("Delete");
				ImGui::PopStyleColor();
			}
			return result;
		});
	}
	
}

