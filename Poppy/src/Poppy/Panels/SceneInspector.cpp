#define IMGUI_DEFINE_MATH_OPERATORS

#include "SceneInspector.hpp"

#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/SelectionContext.hpp"
#include "Poppy/Debug.hpp"

#include "Bloom/Scene/Scene.hpp"

#include <utl/vector.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stack.hpp>

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
		
		displayEntity(EntityID{});
		performHierarchyUpdate();
		
		
		
		if (ImGui::BeginPopupContextWindow ("Context Menu")) // <-- use last item id as popup id
		{
			if (ImGui::MenuItem("Create Entity")) {
				scene()->createEntity("Entity");
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		
		
		
		
		// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Hierarchy Error", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			
			auto [child, parent] = hierarchyUpdate;
			if (child) {
			ImGui::Text("Can't attach \'%s\' to it's descendend \'%s\'. Detach \'%s\' first.",
						scene()->getComponent<TagComponent>(child).name.data(),
						scene()->getComponent<TagComponent>(parent).name.data(),
						scene()->getComponent<TagComponent>(parent).name.data());
			ImGui::Separator();
			}
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("Dismiss", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}
	}
	
	void SceneInspector::displayEntity(bloom::EntityID e) {
		using namespace bloom;
		
		std::string_view const name = e ? scene()->getComponent<bloom::TagComponent>(e).name : std::string_view("Root");
		
		ImGuiTreeNodeFlags flags = 0;
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		
		bool const selected = selection()->isSelected(e);
		
		if (selected) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		
		if (e && expanded(e)) {
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}
		
		if (e && scene()->isLeaf(e)) {
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
		
		bool const nodeExpanded = ImGui::TreeNodeEx((void*)e.raw(), flags, "%s",
													utl::format("{} [0x{:x}]",
																name.data(),
																e.raw()).data());
		if (e)
			setExpanded(e, nodeExpanded);
		
		dragDropSource(e);
		dragDropTarget(e);
		
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && e) {
			selection()->select(e);
		}
		
		if (nodeExpanded) {
			auto const children = e ? scene()->gatherChildren(e) : scene()->gatherRoots();
			for (auto c: children) {
				displayEntity(c);
			}
			ImGui::TreePop();
		}
	}
	
	void SceneInspector::dragDropSource(bloom::EntityID child) {
		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("DD-Entity-Hierarchy-View",
									  &child, sizeof child);
			displayEntity(child);
			ImGui::EndDragDropSource();
		}
	}
	
	void SceneInspector::dragDropTarget(bloom::EntityID parent) {
		using namespace bloom;
		if (ImGui::BeginDragDropTarget()) {
			auto* const payload = ImGui::AcceptDragDropPayload("DD-Entity-Hierarchy-View");
			if (payload && payload->IsDelivery()) {
				EntityID child;
				std::memcpy(&child, payload->Data, sizeof child);
				deferHierarchyUpdate(child, parent);
			}
			ImGui::EndDragDropTarget();
		}
	}
	
	bool SceneInspector::expanded(bloom::EntityID id) const {
		auto const index = id.raw();
		if (index >= _expanded.size()) {
			_expanded.resize(index + 1);
		}
		return _expanded[index];
	}
	
	void SceneInspector::setExpanded(bloom::EntityID id, bool value) {
		auto const index = id.raw();
		if (index >= _expanded.size()) {
			_expanded.resize(index + 1);
		}
		_expanded[index] = value;
	}
	
	void SceneInspector::deferHierarchyUpdate(bloom::EntityID child, bloom::EntityID parent) {
		poppyAssert(!hasHierarchyUpdate);
		hierarchyUpdate = { child, parent };
		hasHierarchyUpdate = true;
	}
	
	void SceneInspector::performHierarchyUpdate() {
		if (!hasHierarchyUpdate) {
			return;
		}
		auto [child, parent] = hierarchyUpdate;
		hasHierarchyUpdate = false;
		
		/**
		 We need to make sure that we don't attach an entity to it's own descendend
		 */
		if (child && !scene()->descendsFrom(parent, child)) {
			scene()->unparent(child);
			if (parent) {
				scene()->parent(child, parent);
			}
		}
		else {
			ImGui::OpenPopup("Hierarchy Error");
		}
	}
	
}
