#include "SceneInspector.hpp"

#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/SelectionContext.hpp"

#include <utl/vector.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stdio.hpp>

using namespace mtl::short_types;

namespace poppy {
	
	SceneInspector::SceneInspector(bloom::Scene* scene,
								   SelectionContext* selection):
		Panel("Scene Inspector"),
		scene(scene),
		selection(selection)
	{
		
	}
	
	void SceneInspector::display() {
		using namespace bloom;
		auto roots = gatherRootEntities();
		
		displayHierachyLevel(roots);
	}
	
	void SceneInspector::displayHierachyLevel(std::span<bloom::EntityID const> entities) {
		for (auto e: entities) {
			auto& tag = scene->getComponent<bloom::TagComponent>(e);
			
			ImGuiTreeNodeFlags flags = 0;
			flags |= ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
			
			bool const selected = selection->isSelected(e);
			
			if (selected) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			
			bool const nodeExpanded = ImGui::TreeNodeEx((void*)(intptr_t)e.value(),
														flags, "%s", tag.name.data());
			
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				selection->select(e);
			}
			
			if (nodeExpanded) {
				if (scene->hasComponent<bloom::ChildrenComponent>(e)) {
					auto& children = scene->getComponent<bloom::ChildrenComponent>(e);
					displayHierachyLevel(children.entities);
				}
				ImGui::TreePop();
			}
		}
	}
	
	utl::small_vector<bloom::EntityID> SceneInspector::gatherRootEntities() {
		utl::small_vector<bloom::EntityID> roots;
		for (auto [entity, parent]: scene->view<bloom::ParentComponent>().each()) {
			if (!parent.entity) {
				roots.push_back(bloom::EntityID(entity));
			}
		}
		return roots;
	}
	
	
	
}
