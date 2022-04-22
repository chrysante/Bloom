#include "SelectionContext.hpp"

namespace poppy {
	
	void SelectionContext::select(bloom::EntityID entity) {
		clear();
		_ids.push_back(entity);
	}
	
	void SelectionContext::addSelect(bloom::EntityID entity) {
		if (isSelected(entity)) {
			return;
		}
		_ids.push_back(entity);
	}
	
	void SelectionContext::deselect(bloom::EntityID entity) {
		auto itr = std::find(_ids.begin(), _ids.end(), entity);
		if (itr == _ids.end()) {
			return;
		}
		_ids.erase(itr);
	}
	
	void SelectionContext::clear() {
		_ids.clear();
	}
	
	bool SelectionContext::isSelected(bloom::EntityID entity) const {
		return std::find(_ids.begin(), _ids.end(), entity) != _ids.end();
	}
	
	
}
