#include "Application.hpp"

#include "Debug.hpp"
#include <iostream>

#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Graphics/Renderer.hpp"

BLOOM_API BLOOM_WEAK bloom::Application* createBloomApplication() {
	std::cerr << "createBloomApplication() needs to be overriden by client" << std::endl;
	std::terminate();
}

namespace bloom {
	
	Application::~Application() = default;
	
	void Application::doInit() {
		_renderer = utl::make_unique_ref<Renderer>();
		_renderer->init(_renderContext.get());
		// call to base
		this->init();
	}
	
	void Application::doShutdown() {
		// call to base
		this->shutdown();
	}
	
	void Application::doUpdate() {
		_updateTimer.update();
		// call to base
		this->update(getUpdateTime());
	}
	
	void Application::doRender() {
		_renderTimer.update();
		// call to base
		this->render(getRenderTime());
	}
	
	void Application::doTickMainThread() {
		flushEventBuffer();
		
		doUpdate();
		doRender();
		_input._clearOffsets();
	}
	
	void Application::handleEvent(Event const& event,
								  void* nativeEvent)
	{
		_eventBuffer.push_back(event);
	}
	
	void Application::flushEventBuffer() {
		for (auto& event: _eventBuffer) {
			_input._setFromEvent(event);
			this->onEvent(event);
		}
		_eventBuffer.clear();
	}
	
	void internal::AppInternals::init(Application* app, utl::unique_ref<RenderContext> renderContext) {
		app->_renderContext = std::move(renderContext);
		app->doInit();
	}
	
}
