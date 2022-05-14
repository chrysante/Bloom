#pragma once

#include <string>
#include "Bloom/Core/Base.hpp"

namespace bloom {
	
	class Scene;
	class AssetManager;
	
	BLOOM_API std::string serialize(Scene const*);
	
	BLOOM_API void deserialize(std::string, Scene*, AssetManager*);
	
}
