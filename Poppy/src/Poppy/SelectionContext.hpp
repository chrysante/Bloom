#pragma once

#include <utl/vector.hpp>
#include <span>

#include "Bloom/Scene/Entity.hpp"

namespace poppy {
	
	class SelectionContext {
	public:
		std::span<bloom::EntityID const> ids() const { return _ids; }
		
		bool empty() const { return _ids.empty(); }
		
		void select(bloom::EntityID);
		void addSelect(bloom::EntityID);
		void deselect(bloom::EntityID);
		void clear();
		
		bool isSelected(bloom::EntityID) const;
		
	private:
		utl::vector<bloom::EntityID> _ids;
	};
	
}
