#include "Application.hpp"

#include "Debug.hpp"
#include <iostream>

#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Graphics/Renderer.hpp"

#include "Bloom/Scene/SceneSystem.hpp"

#include "Bloom/Assets/AssetManager.hpp"

BLOOM_API BLOOM_WEAK bloom::Application* createBloomApplication() {
	std::cerr << "createBloomApplication() must be overriden by client" << std::endl;
	std::terminate();
}

namespace bloom {
	
	Application::~Application() {
		
	}
	
	void Application::doInit() {
		_renderer = utl::make_unique_ref<Renderer>();
		_renderer->init(_renderContext.get());
		_assetManager = createAssetManager();
		_assetManager->setRenderContext(_renderContext.get());
		
		_sceneSystem = utl::make_unique_ref<SceneSystem>();
		
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
	
	void Application::handleEvent(Event const& event) {
		_eventBuffer.push_back(event);
	}
	
	void Application::flushEventBuffer() {
		for (auto& event: _eventBuffer) {
			_input._setFromEvent(event);
			this->onEvent(event);
		}
		_eventBuffer.clear();
	}
	
	utl::unique_ref<AssetManager> Application::createAssetManager() {
		return utl::make_unique_ref<AssetManager>();
	}
	
	void internal::AppInternals::init(Application* app, utl::unique_ref<RenderContext> renderContext) {
		app->_renderContext = std::move(renderContext);
		app->doInit();
	}
	
}
