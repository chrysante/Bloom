#pragma once

#include "ComponentBase.hpp"
#include "Bloom/ScriptEngine/ScriptEngine.hpp"

namespace bloom {
	
	struct BLOOM_API ScriptComponent {
		BLOOM_REGISTER_COMPONENT("Script");
		std::string className;
		std::shared_ptr<ScriptObject> object;
	};
	
}

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::ScriptComponent,
						   className);


#endif
