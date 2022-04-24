#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

#include "Components.hpp"
#include "Entity.hpp"

#include <entt/entt.hpp>
#include <mtl/mtl.hpp>
#include <string>

namespace bloom {
	
	class BLOOM_API Scene {
	public:
		EntityID createEntity(std::string_view name);
		
		template <typename Component>
		void addComponent(EntityID entity, Component const& component) {
			bloomExpect(!hasComponent<Component>(entity), "Component already present");
			_registry.emplace<Component>(entity.value(), component);
		}
		template <typename Component>
		void addComponent(EntityID entity, Component& component) {
			addComponent<Component>(entity, const_cast<Component const&>(component));
		}
		template <typename Component>
		void addComponent(EntityID entity, Component&& component) {
			bloomExpect(!hasComponent<Component>(entity), "Component already present");
			_registry.emplace<std::decay_t<Component>>(entity.value(), std::move(component));
		}
		
		template <typename Component>
		void removeComponent(EntityID entity) {
			bloomExpect(hasComponent<Component>(entity), "Component not present");
			_registry.remove<Component>(entity.value());
		}
		
		template <typename Component>
		bool hasComponent(EntityID entity) const {
			return _registry.any_of<Component>(entity.value());
		}
		
		template <typename Component>
		Component& getComponent(EntityID entity) {
			bloomExpect(hasComponent<Component>(entity), "Component not present");
			return _registry.get<Component>(entity.value());
		}
		
		template <typename... Components>
		auto view() { return _registry.view<Components...>(); }
		
	private:
		entt::registry _registry;
	};
	
}
