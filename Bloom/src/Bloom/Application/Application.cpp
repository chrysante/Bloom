#include "Application.hpp"

#include "Debug.hpp"
#include <iostream>

#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Graphics/Renderer.hpp"
#include "Bloom/Assets/AssetManager.hpp"
#include "Bloom/Scene/SceneSystem.hpp"
#include "Bloom/ScriptEngine/ScriptEngine.hpp"

BLOOM_API BLOOM_WEAK bloom::Application* createBloomApplication() {
	std::cerr << "createBloomApplication() must be overriden by client" << std::endl;
	std::terminate();
}

namespace bloom {
	
	Application* Application::_instance = nullptr;
	
	Application::Application() {
		_instance = this;
	}
	
	Application::~Application() {
		
	}
	
	utl::listener_id Application::addEventListener(utl::listener l) {
		return _messenger.register_listener(std::move(l));
		
	}
	
	void Application::addStaticEventListener(utl::listener l) {
		_listenerIDs.insert(addEventListener(std::move(l)));
	}
	
	void Application::doInit() {
		_scriptEngine = utl::make_ref<ScriptEngine>();
		_sceneSystem = utl::make_ref<SceneSystem>();
		_sceneSystem->init();
		
		
		_renderer = utl::make_ref<Renderer>();
		_renderer->init(_renderContext.get());
		_assetManager = createAssetManager();
		_assetManager->setRenderContext(_renderContext.get());
		
		// call to base
		this->init();
		
		addStaticEventListener([this](InputEvent event) {
			_input._setFromEvent(event);
			this->onInputEvent(event);
		});
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
	
	void Application::handleInputEvent(InputEvent const& event) {
		_messenger.send_message(event);
	}
	
	void Application::flushEventBuffer() {
		_messenger.flush();
	}
	
	utl::unique_ref<AssetManager> Application::createAssetManager() {
		return utl::make_unique_ref<AssetManager>();
	}
	
	void internal::AppInternals::init(Application* app, utl::unique_ref<RenderContext> renderContext) {
		app->_renderContext = std::move(renderContext);
		app->doInit();
	}
	
}
