#include "Renderer.hpp"
#include "ForwardRenderer.hpp"

#include "Bloom/Application/Application.hpp"

namespace bloom {
	
	std::unique_ptr<Renderer> createForwardRenderer(Application& application) {
		return std::make_unique<ForwardRenderer>(application.makeReciever());
	}
	
}
