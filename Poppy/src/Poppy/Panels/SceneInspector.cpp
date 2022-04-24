#include "SceneInspector.hpp"

#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/SelectionContext.hpp"

#include "Bloom/Scene/Scene.hpp"

#include <utl/vector.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stdio.hpp>

using namespace mtl::short_types;

namespace poppy {
	
	SceneInspector::SceneInspector():
		Panel("Scene Inspector")
	{
		
	}
	
	void SceneInspector::display() {
		if (!scene()) {
			return;
		}
		
		using namespace bloom;
		auto roots = gatherRootEntities();
		
		displayHierachyLevel(roots);
	}
	
	void SceneInspector::displayHierachyLevel(std::span<bloom::EntityID const> entities) {
		using namespace bloom;
		for (auto e: entities) {
			auto& tag = scene()->getComponent<bloom::TagComponent>(e);
			
			ImGuiTreeNodeFlags flags = 0;
			flags |= ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
			
			bool const selected = selection()->isSelected(e);
			
			if (selected) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			
			bool const nodeExpanded = ImGui::TreeNodeEx((void*)(intptr_t)e.value(),
														flags, "%s", tag.name.data());
			
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				selection()->select(e);
			}
			
			if (nodeExpanded) {
				auto const children = gatherChildren(e);
				displayHierachyLevel(children);
				ImGui::TreePop();
			}
		}
	}
	
}
