#pragma once

#include <entt/entt.hpp>

namespace bloom {
	
	struct EntityID {
		friend class Scene;
		
	public:
		EntityID() = default;
		EntityID(entt::entity value): _value(value) {}
		
		entt::entity value() const { return _value; }
		
		explicit operator bool() const { return value() != entt::null; }
		
		friend bool operator==(EntityID const&, EntityID const&) = default;
		
	private:
		entt::entity _value = entt::null;
	};
	
}
