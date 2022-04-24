#pragma once

#include <string>
#include <utl/hashmap.hpp>
#include "Bloom/Graphics/RenderPrimitives.hpp"

namespace bloom { class RenderContext; }

namespace poppy {
	
	struct TextureResource {
		bloom::TextureHandle handle;
	};
	
	class ResourceManager {
	public:
		using Resource = std::variant<TextureResource>;
		
		template <typename T>
		T getResource(std::string location);
		
		void setRenderContext(bloom::RenderContext* rc) { renderContext = rc; }
		
	private:
		template <typename T>
		T loadResource(std::string_view location);
		
	private:
		bloom::RenderContext* renderContext = nullptr;
		utl::hashmap<std::string, Resource> resources;
	};
	
	
}
