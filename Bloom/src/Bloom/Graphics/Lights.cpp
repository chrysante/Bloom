#include "Lights.hpp"

#include <array>

namespace bloom {
	
	BLOOM_API std::string_view toString(LightType type) {
		return std::array {
			"Point Light",
			"Spotlight",
			"Directional Light"
		}[(std::size_t)type];
	}
	
}
