#pragma once

#include "ComponentBase.hpp"

#include "Bloom/Scene/Entity.hpp"

namespace bloom {
	
	struct BLOOM_API HierarchyComponent {
		BLOOM_REGISTER_COMPONENT("Hierarchy");
		
		EntityID parent, prevSibling, nextSibling, firstChild, lastChild;
	};
	
	
}

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::HierarchyComponent,
						   parent,
						   prevSibling,
						   nextSibling,
						   firstChild,
						   lastChild);

#endif
