#define IMGUI_DEFINE_MATH_OPERATORS

#include "SceneOutliner.hpp"

#include "Poppy/UI/ImGuiHelpers.hpp"
#include "Poppy/Editor/Editor.hpp"
#include "Poppy/Editor/SelectionContext.hpp"
#include "Poppy/Core/Debug.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"

#include <utl/vector.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stack.hpp>

using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
	
	POPPY_REGISTER_VIEW(SceneOutliner, "Scene Outliner");
	
	SceneOutliner::SceneOutliner():
		View("Scene Inspector"), BasicSceneInspector(this)
	{
		
	}
	
	void SceneOutliner::frame() {
		if (scenes().empty()) {
			displayEmptyWithReason("No active Scene");
			return;
		}
		
		for (auto scene: scenes()) {
			displayScene(*scene);
		}
		performUpdates();
		
		// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Hierarchy Error", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			
			auto [child, parent] = hierarchyUpdate;
			if (child) {
			ImGui::Text("Can't attach \'%s\' to it's descendend \'%s\'. Detach \'%s\' first.",
						child.get<TagComponent>().name.data(),
						parent.get<TagComponent>().name.data(),
						parent.get<TagComponent>().name.data());
			ImGui::Separator();
			}
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("Dismiss", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}
	}
	
	void SceneOutliner::treeNode(TreeNodeDescription const& desc, auto&& block) {
		ImGuiTreeNodeFlags flags = 0;
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		
		if (desc.selected) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		
		if (desc.expanded) {
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}
		
		if (desc.isLeaf) {
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
		
		bool const nodeExpanded = ImGui::TreeNodeEx((void*)desc.id, flags, "%s", desc.name.data());
		
		block(nodeExpanded);
	}
	
	void SceneOutliner::displayScene(bloom::Scene& scene) {
		TreeNodeDescription desc;
		desc.id = (std::size_t)&scene;
		desc.selected = false;
		desc.expanded = expanded(&scene);
		desc.isLeaf = scene.empty();
		desc.name = scene.name();
		
		treeNode(desc, [&](bool isExpanded) {
			ImGui::PushID(desc.id);
			ImGui::OpenPopupOnItemClick("Context Menu");
			if (ImGui::BeginPopup("Context Menu")) {
				if (ImGui::MenuItem("Create Entity")) {
					scene.createEntity("Entity");
//					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Unload")) {
					editor().coreSystems().sceneSystem().unloadScene(scene.handle().id());
//					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
			
			setExpanded(&scene, isExpanded);
			
			dragDropTarget(EntityHandle(EntityID{}, &scene));
			
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
//				selection().select(e);
			}
			
			if (isExpanded) {
				for (auto id: scene.gatherRoots()) {
					displayEntity(scene.getHandle(id));
				}
				ImGui::TreePop();
			}
		});
	}
	
	void SceneOutliner::displayEntity(bloom::EntityHandle e) {
		poppyAssert(!!e);
		
		TreeNodeDescription desc;
		desc.id = e.raw();
		desc.selected = selection().isSelected(e);
		desc.expanded = expanded(e);
		desc.isLeaf = e.scene().isLeaf(e);
		desc.name = e.get<bloom::TagComponent>().name;
		
		treeNode(desc, [&](bool isExpanded) {
			ImGui::PushID(desc.id);
			ImGui::OpenPopupOnItemClick("Context Menu");
			if (ImGui::BeginPopup("Context Menu")) {
				if (ImGui::MenuItem("Delete Entity")) {
					toDelete = e;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
			
			setExpanded(e, isExpanded);
			
			dragDropSource(e);
			dragDropTarget(e);
			
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				selection().select(e);
			}
			
			if (isExpanded) {
				auto const children = e.scene().gatherChildren(e);
				for (auto c: children) {
					displayEntity(e.scene().getHandle(c));
				}
				ImGui::TreePop();
			}
		});
	}
	
	void SceneOutliner::dragDropSource(bloom::EntityHandle child) {
		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("DD-Entity-Hierarchy-View",
									  &child, sizeof child);
			displayEntity(child);
			ImGui::EndDragDropSource();
		}
	}
	
	void SceneOutliner::dragDropTarget(bloom::EntityHandle parent) {
		using namespace bloom;
		if (ImGui::BeginDragDropTarget()) {
			auto* const payload = ImGui::AcceptDragDropPayload("DD-Entity-Hierarchy-View");
			if (payload && payload->IsDelivery()) {
				EntityHandle child;
				std::memcpy(&child, payload->Data, sizeof child);
				deferHierarchyUpdate(child, parent);
			}
			ImGui::EndDragDropTarget();
		}
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
	
	void SceneOutliner::performUpdates() {
		performHierarchyUpdate();
		performDeletion();
	}
	
	void SceneOutliner::performHierarchyUpdate() {
		if (!hasHierarchyUpdate) {
			return;
		}
		auto [child, parent] = hierarchyUpdate;
		hasHierarchyUpdate = false;
		
		if (&child.scene() != &parent.scene()) {
			return;
		}
		
		/**
		 We need to make sure that we don't attach an entity to it's own descendend
		 */
		auto& scene = child.scene();
		if (child && !scene.descendsFrom(parent, child)) {
			scene.unparent(child);
			if (parent) {
				scene.parent(child, parent);
				setExpanded(parent, true);
			}
		}
		else {
			ImGui::OpenPopup("Hierarchy Error");
		}
	}
	
	void SceneOutliner::deferHierarchyUpdate(bloom::EntityHandle child, bloom::EntityHandle parent) {
		poppyAssert(!hasHierarchyUpdate);
		hierarchyUpdate = { child, parent };
		hasHierarchyUpdate = true;
	}
	
	void SceneOutliner::performDeletion() {
		if (toDelete) {
			toDelete.scene().deleteEntity(toDelete);
			selection().deselect(toDelete);
			toDelete = EntityHandle{};
		}
	}
	
}
