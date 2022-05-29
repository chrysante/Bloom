#pragma once

#include <utl/vector.hpp>
#include <utl/utility.hpp>
#include <span>

#include "Bloom/Application/MessageSystem.hpp"
#include "Bloom/Scene/Entity.hpp"

namespace poppy {
	
	class SelectionContext: bloom::Reciever {
	public:
		explicit SelectionContext(bloom::Reciever);
		
		std::span<bloom::EntityHandle const> entities() const {
			return mEntities;
		}
		
		bool empty() const { return mEntities.empty(); }
		
		void select(bloom::EntityHandle);
		void addSelect(bloom::EntityHandle);
		void deselect(bloom::EntityHandle);
		void clear();
		
		bool isSelected(bloom::EntityHandle) const;
		
	private:
		utl::vector<bloom::EntityHandle> mEntities;
	};
	
}
