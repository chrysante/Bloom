#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

#include "Components/AllComponents.hpp"
#include "Entity.hpp"

#include <entt/entt.hpp>
#include <mtl/mtl.hpp>
#include <string>

namespace bloom {
	
	class BLOOM_API Scene {
	public:
		EntityID createEntity(std::string_view name);
		EntityID createEmptyEntity();
		EntityID createEmptyEntity(EntityID hint);
		
		
		EntityHandle getHandle(EntityID id) { return EntityHandle(id, this); }
		ConstEntityHandle getHandle(EntityID id) const { return ConstEntityHandle(id, this); }
		
		EntityID cloneEntity(EntityID);
		
		void deleteEntity(EntityID);
		
		template <Component T>
		bool hasComponent(EntityID entity) const {
			return _registry.any_of<T>(entity.value());
		}
		
		template <Component T>
		T& getComponent(EntityID entity) {
			bloomExpect(hasComponent<T>(entity), "Component not present");
			return _registry.get<T>(entity.value());
		}
		template <Component T>
		T const& getComponent(EntityID entity) const {
			bloomExpect(hasComponent<T>(entity), "Component not present");
			return _registry.get<T>(entity.value());
		}
		
		template <Component T>
		void addComponent(EntityID entity, T&& component) {
			bloomExpect(!hasComponent<std::decay_t<T>>(entity), "Component already present");
			_registry.emplace<std::decay_t<T>>(entity.value(), UTL_FORWARD(component));
		}
		
		template <Component T>
		void removeComponent(EntityID entity) {
			bloomExpect(hasComponent<T>(entity), "Component not present");
			_registry.remove<T>(entity.value());
		}
		
		void clear() { _registry.clear(); }
		
		bool empty() const { return _registry.empty(); }
		
		template <typename... Components>
		auto view() { return _registry.view<Components...>(); }
		template <typename... Components>
		auto view() const { return _registry.view<Components...>(); }
		
		
		auto each(std::invocable<entt::entity> auto&& f) {
			_registry.each(UTL_FORWARD(f));
		}
		auto each(std::invocable<entt::entity> auto&& f) const {
			_registry.each(UTL_FORWARD(f));
		}
		
		Scene copy();
		
		/// MARK: Hierarchy functionality. Maybe extract this later
		void parent(EntityID child, EntityID parent);
		void unparent(EntityID);
		
		// returns true if 'descendend' and 'ancestor' are the same
		bool descendsFrom(EntityID descendend, EntityID ancestor) const;
		utl::small_vector<EntityID> gatherRoots() const;
		utl::small_vector<EntityID> gatherChildren(EntityID parent) const;
		
		bool isLeaf(EntityID) const;
		
		mtl::float4x4 calculateTransformRelativeToWorld(EntityID) const;
		
	private:
		entt::registry _registry;
	};
	
}
