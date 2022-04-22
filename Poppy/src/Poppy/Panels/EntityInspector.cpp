#include "EntityInspector.hpp"

#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/SelectionContext.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>

using namespace mtl::short_types;


static void ComponentHeader(char const* name,
							poppy::FontWeight = poppy::FontWeight::semibold,
							poppy::FontStyle = poppy::FontStyle::roman);

namespace poppy {
	
	EntityInspector::EntityInspector(bloom::Scene* scene,
									 SelectionContext* selection):
		Panel("Entity Inspector"),
		scene(scene),
		selection(selection)
	{
		
	}
	
	void EntityInspector::display() {
		bloom::EntityID const activeEntity = selection->empty() ? bloom::EntityID{} : selection->ids()[0];
		auto const entity = activeEntity;
		
		if (!entity) {
			return;
		}
		
		using namespace bloom;
		
		if (scene->hasComponent<TagComponent>(entity)) {
			std::string_view const name = scene->getComponent<TagComponent>(entity).name;
			
			ComponentHeader(name.data(), FontWeight::bold);
		}
		ImGui::Separator();
		
		if (scene->hasComponent<TransformComponent>(entity)) {
			inspectTransform(entity);
		}
		
		if (scene->hasComponent<LightComponent>(entity)) {
			inspectLight(entity);
		}
		
		ImGui::Separator();
	}
	
	static void inputPosition(float3& position, float labelWidth) {
		auto const cursorY = ImGui::GetCursorPos().y;
		ImGui::Text("Position");
		
		ImGui::SetCursorPos({ labelWidth + GImGui->Style.FramePadding.x * 2, cursorY });
		
		dragFloat3Pretty(position.data(), "position");
	}
	
	static void inputOrientation(quaternion_float& orientation, float labelWidth) {
		auto cursorY = ImGui::GetCursorPos().y;
		ImGui::Text("Orientation");
		
		ImGui::SetCursorPos({ labelWidth + GImGui->Style.FramePadding.x * 2, cursorY });
		
		float3 euler = mtl::to_euler(orientation) * 180;
		if (dragFloat3Pretty(euler.data(), "orientation")) {
			orientation = mtl::to_quaternion(euler / 180);
		}
	}
	
	static void inputScale(float3& scale, float labelWidth) {
		auto const cursorY = ImGui::GetCursorPos().y;
		ImGui::Text("Scale");
		
		ImGui::SetCursorPos({ labelWidth + GImGui->Style.FramePadding.x * 2, cursorY });
		
		dragFloat3Pretty(scale.data(), "scale", 0.02);
	}
	
	void EntityInspector::inspectTransform(bloom::EntityID entity) {
		using namespace bloom;
		auto& transform = scene->getComponent<TransformComponent>(entity);
	
		auto const labelWidth = std::max({
			ImGui::CalcTextSize("Position").x,
			ImGui::CalcTextSize("Orientation").x,
			ImGui::CalcTextSize("Rotation").x
		});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		inputPosition(transform.position, labelWidth);
		inputOrientation(transform.orientation, labelWidth);
		inputScale(transform.scale, labelWidth);
		ImGui::PopStyleVar();
	}
	
	void EntityInspector::inspectLight(bloom::EntityID entity) {
		using namespace bloom;
		auto& light = scene->getComponent<LightComponent>(entity);
		
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
	}
	
	void EntityInspector::inspectPointLight(bloom::PointLight& light) {
		ComponentHeader("Point Light");
		ImGui::ColorPicker3("##Point-Light-Color", light.color.data());
		ImGui::DragFloat("Intensity", &light.intensity, 100, 0, FLT_MAX, "%f");
	}
	
	void EntityInspector::inspectSpotLight(bloom::SpotLight& light) {
		ComponentHeader("Spot Light");
		ImGui::ColorPicker3("##Spot-Light-Color", light.color.data());
		ImGui::DragFloat("Intensity", &light.intensity, 1, 0);
		
		
		float const inner = light.innerCutoff;
		float const outer = light.outerCutoff;
		
		float angle = (inner + outer) / 2;
		float falloff = (outer - inner) / 2;
		
		ImGui::SliderFloat("Angle", &angle, 0, 1);
		ImGui::SliderFloat("Falloff", &falloff, 0, 0.2);
		
		light.innerCutoff = angle - falloff;
		light.outerCutoff = angle + falloff;
		
	}
	
	
}


static void ComponentHeader(char const* name, poppy::FontWeight weight, poppy::FontStyle style) {
	using namespace poppy;
	auto* font = poppy::ImGuiContext::instance().getFont(weight, style);
	ImGui::PushFont((ImFont*)font);
	ImGui::Text("%s", name);
	ImGui::PopFont();
}
