#include "Scene.hpp"

#include "Components.hpp"

#include <utl/stack.hpp>

namespace bloom {
	
	EntityID Scene::createEntity(std::string_view name) {
		EntityID const entity = EntityID(_registry.create());
		addComponent(entity, TransformComponent{});
		addComponent(entity, TransformMatrixComponent{});
		addComponent(entity, TagComponent{ std::string(name) });
		addComponent(entity, HierarchyComponent{});

		return entity;
	}
	
	[[ maybe_unused ]] static void sanitizeHierachy(Scene* scene) {
		auto view = scene->view<HierarchyComponent>();
		view.each([&](auto const e, HierarchyComponent entity) {
			EntityID const entityID = e;
			// sanitize child -> parent relationship
			if (entity.parent) {
				EntityID const parentID = entity.parent;
				auto const parentsChildren = scene->gatherChildren(parentID);
				if (std::find(parentsChildren.begin(), parentsChildren.end(), entityID) == parentsChildren.end()) {
					bloomLog(fatal,
							 "Entity {0} [{1}] thinks Entity {2} [{3}] is its parent but {2} doesn't know about it.",
							 entityID, scene->getComponent<TagComponent>(entityID).name,
							 parentID, scene->getComponent<TagComponent>(parentID).name);
					bloomDebugbreak();
				}
			}
			
			// sanitize parent -> children relationship
			if (entity.firstChild) {
				bloomAssert(!!entity.lastChild);
				auto const ourChildren = scene->gatherChildren(entityID);
				for (auto const childID: ourChildren) {
					auto const child = scene->getComponent<HierarchyComponent>(childID);
					if (child.parent != entityID) {
						bloomLog(fatal,
								 "Entity {0} [{1}] thinks it's a parent but supposed child {0} [{1}] doesn't know about it.",
								 entityID, scene->getComponent<TagComponent>(entityID).name,
								 childID, scene->getComponent<TagComponent>(childID).name);
						bloomDebugbreak();
					}
				}
			}
			
			// sanitize relationship among siblings
			if (entity.prevSibling) {
				bloomAssert(!!entity.nextSibling);
				auto const leftSibling = scene->getComponent<HierarchyComponent>(entity.prevSibling);
				auto const rightSibling = scene->getComponent<HierarchyComponent>(entity.nextSibling);
				bloomAssert(leftSibling.nextSibling == entityID,
							"Our left sibling doesn't know about us");
				bloomAssert(rightSibling.prevSibling == entityID,
							"Our right sibling doesn't know about us");
			}
		});
	}
	
	void Scene::parent(EntityID c, EntityID p) {
		HierarchyComponent& parent = getComponent<HierarchyComponent>(p);
		HierarchyComponent& newChild = getComponent<HierarchyComponent>(c);
		bloomExpect(!newChild.parent);
		
		newChild.parent = p;
		
		if (!parent.firstChild) { // case parent has no children yet
			bloomAssert(!parent.lastChild);
			parent.firstChild = c;
			parent.lastChild = c;
			newChild.prevSibling = c;
			newChild.nextSibling = c;
		}
		else { // parent already has children
			EntityID const listBegin = parent.firstChild;
			EntityID const listEnd = parent.lastChild;
		
			HierarchyComponent& leftChild = getComponent<HierarchyComponent>(listBegin);
			HierarchyComponent& rightChild = getComponent<HierarchyComponent>(listEnd);
			
			rightChild.nextSibling = c;
			leftChild.prevSibling = c;
			
			newChild.prevSibling = listEnd;
			newChild.nextSibling = listBegin;
			
			parent.lastChild = c;
		}
		
		auto& t = getComponent<TransformComponent>(c);
		
		mtl::float4x4 const parentWorldTransform = calculateTransformRelativeToWorld(p);
		mtl::float4x4 const childLocalTransform = mtl::inverse(parentWorldTransform) * t.calculate();
		
		t = TransformComponent::fromMatrix(childLocalTransform);
		
#if BLOOM_DEBUGLEVEL
		sanitizeHierachy(this);
#endif
	}
	
	void Scene::unparent(EntityID c) {
		auto& child = getComponent<HierarchyComponent>(c);
		
		if (!child.parent) {
			return;
		}
		
		auto& parent = getComponent<HierarchyComponent>(child.parent);
		auto& leftSibling = getComponent<HierarchyComponent>(child.prevSibling);
		auto& rightSibling = getComponent<HierarchyComponent>(child.nextSibling);
		
		/// Remove element from circular linked list
		leftSibling.nextSibling = child.nextSibling;
		rightSibling.prevSibling = child.prevSibling;
		
		if (c == parent.firstChild) { // adjust begin/end nodes if we were those
			parent.firstChild = child.nextSibling;
		}
		if (c == parent.lastChild) {
			parent.lastChild = child.prevSibling;
		}
		
		// either both are true or neither is true
		bloomAssert(! ((parent.firstChild == c) ^ (parent.lastChild == c)));
		
		// only one element was in the list
		if (parent.firstChild == c) {
			bloomAssert(c == child.prevSibling);
			bloomAssert(c == child.nextSibling);
			
			parent.lastChild = {};
			parent.firstChild = {};
		}
		
		
		auto& t = getComponent<TransformComponent>(c);
		
		mtl::float4x4 const parentWorldTransform = calculateTransformRelativeToWorld(child.parent);
		mtl::float4x4 const childWorldTransform = parentWorldTransform * t.calculate();
		
		t = TransformComponent::fromMatrix(childWorldTransform);
		
		child.parent = {};
		child.prevSibling = {};
		child.nextSibling = {};
		
#if BLOOM_DEBUGLEVEL
		sanitizeHierachy(this);
#endif
	}
	
	bool Scene::descendsFrom(EntityID descended, EntityID ancestor) const {
		if (!descended) {
			return false;
		}
		bloomExpect(hasComponent<HierarchyComponent>(descended));
//		bloomExpect(hasComponent<HierarchyComponent>(ancestor)); // not really necessary but here for good measure. Edit: entity may be null
		
		// walk up the tree until we find our potentil ancestor or the root
		do {
			if (descended == ancestor) {
				return true;
			}
			descended = getComponent<HierarchyComponent>(descended).parent;
		} while (descended);
		return false;
	}

	utl::small_vector<EntityID> Scene::gatherRoots() const {
		utl::small_vector<EntityID> roots;
		for (auto [entity, hierachy]: view<HierarchyComponent>().each()) {
			if (!hierachy.parent) {
				roots.push_back(EntityID(entity));
			}
		}
		return roots;
	}
	
	utl::small_vector<EntityID> Scene::gatherChildren(EntityID parent) const {
		if (!hasComponent<HierarchyComponent>(parent)) {
			return {};
		}
		auto const& hierarchy = getComponent<HierarchyComponent>(parent);
		auto current = hierarchy.firstChild;
		auto const end = hierarchy.lastChild;
		
		utl::small_vector<EntityID> children;
		while (current) {
			children.push_back(current);
			if (current == end) {
				break;
			}
			current = getComponent<HierarchyComponent>(current).nextSibling;
		}
		
		return children;
	}
	
	bool Scene::isLeaf(EntityID entity) const {
		bool const result = !getComponent<HierarchyComponent>(entity).firstChild;
		bloomAssert(result == !getComponent<HierarchyComponent>(entity).lastChild);
		return result;
	}
	
	mtl::float4x4 Scene::calculateTransformRelativeToWorld(EntityID entity) const {
		mtl::float4x4 result = 1;
		
		while (entity) {
			bloomAssert(hasComponent<HierarchyComponent>(entity),
						"This API is supposed to be used with hierarchical entities");
			result *= getComponent<TransformComponent>(entity).calculate();
			entity = getComponent<HierarchyComponent>(entity).parent;
		}
		
		return result;
	}
	
}
