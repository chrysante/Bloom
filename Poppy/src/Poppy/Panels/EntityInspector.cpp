#define IMGUI_DEFINE_MATH_OPERATORS

#include "EntityInspector.hpp"

#include "AssetBrowser.hpp"

#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/SelectionContext.hpp"
#include "Poppy/Debug.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Assets/StaticMeshAsset.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>

using namespace mtl::short_types;

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
		auto const entity = activeEntity;
		
		if (!entity) {
			return;
		}
		
		using namespace bloom;
		
		if (scene()->hasComponent<TagComponent>(entity)) {
			inspectTag(entity);
			ImGui::Separator();
		}
		
		if (scene()->hasComponent<TransformComponent>(entity)) {
			inspectTransform(entity);
			ImGui::Separator();
		}
		
		if (scene()->hasComponent<MeshRenderComponent>(entity)) {
			inspectMesh(entity);
			ImGui::Separator();
		}
		
		if (scene()->hasComponent<LightComponent>(entity)) {
			inspectLight(entity);
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
					auto addComponentButton = [&]<typename Component>(char const* name, EntityID entity){
						auto flags = 0;
						if (scene()->hasComponent<Component>(entity)) {
							flags |= ImGuiSelectableFlags_Disabled;
						}
						if (ImGui::Selectable(name, false, flags)) {
							scene()->addComponent(entity, Component{});
						}
					};
					addComponentButton.operator()<MeshRenderComponent>("Static Mesh", entity);
					addComponentButton.operator()<LightComponent>("Light", entity);
				});
				ImGui::EndCombo();
			}
		});
		
		ImGui::EndChild();
	}
	
	void EntityInspector::inspectTransform(bloom::EntityID entity) {
		using namespace bloom;
		auto& transform = scene()->getComponent<TransformComponent>(entity);
	
		
		if (beginSection("Transform")) {
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
		using namespace bloom;
		auto& meshRenderer = scene()->getComponent<MeshRenderComponent>(entity);
		(void)meshRenderer;
		
		if (beginSection<MeshRenderComponent>("Static Mesh Component", entity)) {
			beginProperty("Mesh");
			ImGui::Button("[Mesh Name Here]");
			recieveMeshDragDrop(entity);
	
			beginProperty("Material");
			ImGui::Button("[Material Name Here]");
			if (auto const recievedAsset = acceptAssetDragDrop(AssetType::material)) {
				poppyLog(trace, "Recieved Asset: {}", assetManager()->getName(*recievedAsset));
			}
			
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
		
		auto* const asset = utl::down_cast<StaticMeshAsset*>(assetManager()->getAssetWeak(handle));
		poppyAssert(asset);
		
		assetManager()->makeAvailable(handle, AssetRepresentation::GPU);
			
		auto& meshRenderer = scene()->getComponent<MeshRenderComponent>(entity);
		meshRenderer.mesh = asset->getRenderMesh();
	}
	
	void EntityInspector::inspectLight(bloom::EntityID entity) {
		using namespace bloom;
		auto& light = scene()->getComponent<LightComponent>(entity);
		
		if (beginSection<LightComponent>("Light Component", entity)) {
			inspectLightType(light);
			inspectLightCommon(light.getCommon(), light.type());
		
			switch (light.type()) {
				case LightType::pointlight:
					inspectPointLight(light.get<PointLight>());
					break;
				case LightType::spotlight:
					inspectSpotLight(light.get<SpotLight>());
					break;
					
				default:
					break;
			}
			
			endSection();
		}
	}
	
	
	void EntityInspector::inspectLightType(bloom::LightComponent& light) {
		using namespace bloom;
		beginProperty("Type");
		auto newType = light.type();
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
		if (newType != light.type()) {
			auto const common = light.getCommon();
			switch (newType) {
				case LightType::pointlight:
					light = PointLight{ common };
					break;
					
				case LightType::spotlight:
					light = SpotLight{ common };
					break;
					
				case LightType::directional:
					light = DirectionalLight{ common };
					break;
					
				default:
					poppyLog(error, "Other Light Types not implemented");
					break;
			}
		}
	}
	
	void EntityInspector::inspectLightCommon(bloom::LightCommon& light, bloom::LightType type) {
		beginProperty("Color");
		ImGui::ColorEdit4("##light-color", light.color.data(),
						  ImGuiColorEditFlags_NoInputs |
						  ImGuiColorEditFlags_NoLabel |
						  ImGuiColorEditFlags_HDR);
		beginProperty("Intensity");
		float const speed = type == bloom::LightType::directional ? 0.01 : 100;
		ImGui::DragFloat("##-intensity", &light.intensity, speed, 0, FLT_MAX, "%f");
	}
	
	void EntityInspector::inspectPointLight(bloom::PointLight& light) {
		beginProperty("Radius");
		ImGui::SliderFloat("##-radius", &light.radius, 0, 100);
	}
	
	void EntityInspector::inspectSpotLight(bloom::SpotLight& light) {
		beginProperty("Radius");
		ImGui::SliderFloat("##-radius", &light.radius, 0, 100);
		
		float const inner = light.innerCutoff;
		float const outer = light.outerCutoff;
		
		float angle = (inner + outer) / 2;
		float falloff = (outer - inner) / 2;
		
		beginProperty("Radius");
		ImGui::SliderFloat("##-angle", &angle, 0, 1);
		beginProperty("Falloff");
		ImGui::SliderFloat("##-falloff", &falloff, 0, 0.2);
		
		light.innerCutoff = angle - falloff;
		light.outerCutoff = angle + falloff;
	}
	
	/// MARK: - Helpers
	template <typename T>
	void EntityInspector::componentHeader(std::string_view name, bloom::EntityID entity) {
		if constexpr (std::is_same_v<T, void>) {
			componentHeaderEx(name, nullptr);
		}
		else {
			componentHeaderEx(name, [&]{
				scene()->removeComponent<T>(entity);
			});
		}
	}
	
	void EntityInspector::componentHeaderEx(std::string_view name,
											utl::function<void()> deleter)
	{
		withFont(FontWeight::semibold, FontStyle::roman, [&]{
			float2 const cursorPos = ImGui::GetCursorPos();
			{
				float4 color = GImGui->Style.Colors[ImGuiCol_Text];
				color.a *= 0.85;
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::Text("%s", name.data());
				ImGui::PopStyleColor();
			}
			
			// 'Delete' Button
			if (deleter) {
				float2 const textSize = ImGui::CalcTextSize("Delete");
				float2 const buttonPos = { cursorPos.x + ImGui::GetWindowWidth() - textSize.x, cursorPos.y };
				
				ImGui::SetCursorPos(buttonPos);
				char deleteButtonLabel[64] = "-delete-button";
				std::strncpy(&deleteButtonLabel[14], name.data(), 49);
				if (ImGui::InvisibleButton(deleteButtonLabel, textSize)) {
					deleter();
				}
				
				float4 const color = GImGui->Style.Colors[ImGuiCol_TextDisabled];
				float4 const hightlightColor = (float4(GImGui->Style.Colors[ImGuiCol_Text]) + color) / 2;
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::IsItemHovered() ? hightlightColor : color);
				ImGui::SetCursorPos(buttonPos);
				ImGui::Text("Delete");
				ImGui::PopStyleColor();
			}
		});
	}
	
	bool EntityInspector::beginSection(std::string_view name) {
		return beginSection<void>(name, bloom::EntityID{});
	}
	
	template <typename T>
	bool EntityInspector::beginSection(std::string_view name, bloom::EntityID entity) {
		componentHeader<T>(name, entity);
		
		bool const open =  ImGui::BeginTable("Property Table", 2,
											 ImGuiTableFlags_Resizable |
											 ImGuiTableFlags_NoBordersInBodyUntilResize);
		if (open) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f); // Default to 100.0f
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);       // Default to 200.0f
		}
		return open;
	}
	
	void EntityInspector::endSection() {
		ImGui::EndTable();
	}
	
	void EntityInspector::beginProperty(std::string_view label) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		
		withFont(FontWeight::light, FontStyle::roman, [&]{
			ImGui::SetCursorPosX(3 * GImGui->Style.WindowPadding.x);
			ImGui::Text("%s", label.data());
		});
		
		ImGui::TableSetColumnIndex(1);
	}
	

	
	
}

