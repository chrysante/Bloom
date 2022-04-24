#include "Scene.hpp"

#include "Components.hpp"

namespace bloom {
	
	EntityID Scene::createEntity(std::string_view name) {
		EntityID const entity = EntityID(_registry.create());
		addComponent(entity, TransformComponent{});
		addComponent(entity, TransformMatrixComponent{});
		addComponent(entity, TagComponent{ std::string(name) });
		addComponent(entity, HierachyComponent{});

//		if (parent) {
//			if (!hasComponent<ChildrenComponent>(parent)) {
//				addComponent(parent, ChildrenComponent{});
//			}
//			auto& children = getComponent<ChildrenComponent>(parent);
//			children.entities.push_back(entity);
//		}
		
		return entity;
	}
	
}
