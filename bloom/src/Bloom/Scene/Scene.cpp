#include "Scene.hpp"

#include "Bloom/Core/Debug.hpp"

#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Hierarchy.hpp"

#include <utl/stack.hpp>
#include <utl/format.hpp>
#include <utl/hashmap.hpp>
#include <yaml-cpp/helpers.hpp>

namespace bloom {
	
	EntityHandle Scene::createEmptyEntity() {
		return createEmptyEntity(EntityHandle{});
	}
	
	EntityHandle Scene::createEmptyEntity(EntityID hint) {
		EntityHandle const entity = EntityHandle(_registry.create(hint.value()), this);
		return entity;
	}
	
	EntityHandle Scene::createEntity(std::string_view name) {
		EntityHandle const entity(_registry.create(), this);
		entity.add(Transform{});
		entity.add(TransformMatrixComponent{});
		entity.add(TagComponent{ std::string(name) });
		entity.add(HierarchyComponent{});

		return { entity, this };
	}
	
	EntityHandle Scene::cloneEntity(EntityID from) {
		EntityHandle const result = createEmptyEntity();
		
		forEachComponent([&]<typename C>(utl::tag<C>) {
			if (hasComponent<C>(from)) {
				C const& c = getComponent<C>(from);
				result.add(c);
			}
		});
		
		getComponent<HierarchyComponent>(result) = {};
		auto const fromHierarchy = getComponent<HierarchyComponent>(from);
		if (fromHierarchy.parent) {
			parent(result, fromHierarchy.parent);
		}
	
		return result;
	}
	
	void Scene::deleteEntity(EntityID id) {
		_registry.destroy(id.value());
	}
	
	Scene Scene::copy() {
		utl::hashmap<EntityID, EntityID> idMap;
		
		Scene result(handle(), std::string(name()));
		_registry.each([&](EntityID fromID) {
			auto const fromHandle = this->getHandle(fromID);
			EntityID const toID = result.createEmptyEntity(fromID);
			auto const toHandle = result.getHandle(toID);
			
			idMap.insert({ fromID, toID });
			forEachComponent([&]<typename C>(utl::tag<C>){
				if (fromHandle.has<C>()) {
					toHandle.add(fromHandle.get<C>());
				}
			});
		});
		
		// copy hierarchy
		for (auto [from, to]: idMap) {
			if (from == to) {
				continue;
			}
			bloomDebugbreak("Probably not going to happen");
			auto fromHandle = this->getHandle(from);
			auto toHandle = result.getHandle(to);
			if (toHandle.has<HierarchyComponent>()) {
				auto const& fromHierarchy = fromHandle.get<HierarchyComponent>();
				auto& toHierarchy = toHandle.get<HierarchyComponent>();
				
				if (fromHierarchy.parent) {
					toHierarchy.parent = idMap[fromHierarchy.parent];
				}
				if (fromHierarchy.firstChild) {
					toHierarchy.firstChild = idMap[fromHierarchy.firstChild];
				}
				if (fromHierarchy.lastChild) {
					toHierarchy.lastChild = idMap[fromHierarchy.lastChild];
				}
				if (fromHierarchy.nextSibling) {
					toHierarchy.nextSibling = idMap[fromHierarchy.nextSibling];
				}
				if (fromHierarchy.prevSibling) {
					toHierarchy.prevSibling = idMap[fromHierarchy.prevSibling];
				}
			}
		}
		
		return result;
	}
	
	/// MARK: Serialize
	template <typename T>
	static void serializeComponent(YAML::Node& node, ConstEntityHandle entity, utl::tag<T>) {
		if (entity.has<T>()) {
			node[T::staticName()] = entity.get<T>();
		}
	}
	
	static void serializeComponent(YAML::Node& node, ConstEntityHandle entity, utl::tag<MeshRendererComponent>) {
		if (entity.has<MeshRendererComponent>()) {
			node[MeshRendererComponent::staticName()] = entity.get<MeshRendererComponent>().serialize();
		}
	}
	
	template <typename T>
	static void deserializeComponent(YAML::Node const& node, EntityHandle entity, AssetManager& assetManager, utl::tag<T>) {
		YAML::Node componentNode = node[T::staticName()];
		if (componentNode.IsDefined()) {
			entity.add(componentNode.as<T>());
		}
	}
	
	static void deserializeComponent(YAML::Node const& node, EntityHandle entity, AssetManager& assetManager, utl::tag<MeshRendererComponent>) {
		if (YAML::Node componentNode = node[MeshRendererComponent::staticName()];
			componentNode.IsDefined())
		{
			entity.add(MeshRendererComponent{});
			entity.get<MeshRendererComponent>().deserialize(componentNode, assetManager);
		}
	}
	
	static YAML::Node serializeEntity(Scene const& scene, EntityID id) {
		YAML::Node node;
		node["ID"] = id.raw();
		auto entity = scene.getHandle(id);
		forEachComponent(except<TransformMatrixComponent>, [&]<typename T>(utl::tag<T>) {
			serializeComponent(node, entity, utl::tag<T>{});
		});
		return node;
	}
	
	static void deserializeEntity(YAML::Node const& node, Scene& scene, AssetManager& assetManager) {
		EntityID const reference(node["ID"].as<EntityID::RawType>());
		EntityID const id = scene.createEmptyEntity(reference);
		auto entity = scene.getHandle(id);
		entity.add(TransformMatrixComponent{});
		bloomAssert(reference == entity);
		forEachComponent(except<TransformMatrixComponent>, [&]<typename T>(utl::tag<T>) {
			deserializeComponent(node, entity, assetManager, utl::tag<T>{});
		});
	}
	
	BLOOM_API YAML::Node Scene::serialize() const {
		YAML::Node root;
		each([&](entt::entity entity){
			root.push_back(serializeEntity(*this, entity));
		});
		return root;
	}

	BLOOM_API void Scene::deserialize(YAML::Node const& root, AssetManager& assetManager) {
		if (!root.IsSequence()) {
			bloomLog(info, "Failed to deserialize Scene: Scene is empty.");
			return;
		}
		
		for (YAML::Node const& node: root) {
			deserializeEntity(node, *this, assetManager);
		}
	}
	
	/// MARK: Hierarchy
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
		
		auto& t = getComponent<Transform>(c);
		
		mtl::float4x4 const parentWorldTransform = calculateTransformRelativeToWorld(p);
		mtl::float4x4 const childLocalTransform = mtl::inverse(parentWorldTransform) * t.calculate();
		
		t = Transform::fromMatrix(childLocalTransform);
		
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
		
		
		auto& t = getComponent<Transform>(c);
		
		mtl::float4x4 const parentWorldTransform = calculateTransformRelativeToWorld(child.parent);
		mtl::float4x4 const childWorldTransform = parentWorldTransform * t.calculate();
		
		t = Transform::fromMatrix(childWorldTransform);
		
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
			result *= getComponent<Transform>(entity).calculate();
			entity = getComponent<HierarchyComponent>(entity).parent;
		}
		
		return result;
	}
	
}
