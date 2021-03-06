#define IMGUI_DEFINE_MATH_OPERATORS
#define UTL_DEFER_MACROS

#include "SceneOutliner.hpp"

#include "Poppy/UI/ImGuiHelpers.hpp"
#include "Poppy/Editor/Editor.hpp"
#include "Poppy/Editor/SelectionContext.hpp"
#include "Poppy/Core/Debug.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"

#include <utl/vector.hpp>
#include <utl/stack.hpp>
#include <utl/scope_guard.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>



using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
	
	POPPY_REGISTER_VIEW(SceneOutliner, "Scene Outliner", {});
	
	SceneOutliner::SceneOutliner():
		BasicSceneInspector(this)
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
		
//		ImGui::InvisibleButton("open-popup-hack", ImGui::GetContentRegionAvail());
//		ImGui::OpenPopupOnItemClick("SceneOutlinerContextMenu");
//		if (ImGui::BeginPopup("SceneOutlinerContextMenu")) {
//
//			ImGui::EndPopup();
//		}
	}
	
	void SceneOutliner::onInput(bloom::InputEvent& e) {

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
			utl_defer { if (isExpanded) ImGui::TreePop(); };
			/* Context Menu */ {
				ImGui::PushID(desc.id);
				utl_defer { ImGui::PopID(); };
				ImGui::OpenPopupOnItemClick("Context Menu");
				if (ImGui::BeginPopup("Context Menu")) {
					utl_defer { ImGui::EndPopup(); };
					if (ImGui::BeginMenu("New Entity")) {
						if (ImGui::MenuItem("Mesh Renderer")) {
							auto entity = scene.createEntity("Mesh Renderer");
							entity.add(MeshRendererComponent{});
						}
						
						ImGui::Separator();
						
						if (ImGui::MenuItem("Point Light")) {
							auto entity = scene.createEntity("Point Light");
							entity.add(PointLightComponent{});
						}
						
						if (ImGui::MenuItem("Spotlight")) {
							auto entity = scene.createEntity("Spotlight");
							entity.add(SpotLightComponent{});
						}
						
						if (ImGui::MenuItem("Directional Light")) {
							auto entity = scene.createEntity("Directional Light");
							entity.add(DirectionalLightComponent{});
						}
						
						if (ImGui::MenuItem("Sky Light")) {
							auto entity = scene.createEntity("Sky Light");
							entity.add(SkyLightComponent{});
						}
						
						ImGui::EndMenu();
					}
					
					
					
					
					
					if (ImGui::MenuItem("Unload")) {
						editor().coreSystems().sceneSystem().unloadScene(scene.handle().id());
						return;
					}
				}
			}
			
			setExpanded(&scene, isExpanded);
			
			dragDropTarget(EntityHandle(EntityID{}, &scene));

			if (isExpanded) {
				for (auto id: scene.gatherRoots()) {
					displayEntity(scene.getHandle(id));
				}
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
			utl_defer {
				if (isExpanded)
					ImGui::TreePop();
			};
			
			/* Context Menu */ {
				ImGui::PushID(desc.id);
				utl_defer{ ImGui::PopID(); };
				ImGui::OpenPopupOnItemClick("Context Menu");
				if (ImGui::BeginPopup("Context Menu")) {
					utl_defer { ImGui::EndPopup(); };
					if (ImGui::MenuItem("Delete Entity")) {
						dispatch(DispatchToken::nextFrame, CustomCommand([this, e]{
							e.scene().deleteEntity(e);
							selection().deselect(e);
						}));
						
						ImGui::CloseCurrentPopup();
					}
				}
			}
			
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
				
				dispatch(DispatchToken::nextFrame, CustomCommand([this, child, parent,
																  window = ImGui::GetCurrentWindow()]{
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
						poppyLog(warning, "Hierarchy Error");
					}
				}));
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
	
}
