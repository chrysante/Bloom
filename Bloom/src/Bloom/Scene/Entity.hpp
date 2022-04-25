#pragma once

#include "Bloom/Core/Base.hpp"

#include <entt/entt.hpp>
#include <iosfwd>
#include <utl/utility.hpp>

namespace bloom {
	
	struct BLOOM_API EntityID {
	public:
		friend class Scene;
		using RawType = std::underlying_type_t<entt::entity>;
		
	public:
		EntityID() = default;
		EntityID(entt::entity value): _value(value) {}
		
		entt::entity value() const { return _value; }
		RawType raw() const { return utl::to_underlying(_value); }
		
		explicit operator bool() const { return value() != entt::null; }
		
		friend bool operator==(EntityID const&, EntityID const&) = default;
		
	private:
		entt::entity _value = entt::null;
	};
	
	BLOOM_API std::ostream& operator<<(std::ostream&,  EntityID);
	
}
