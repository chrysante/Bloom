#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"
#include "InputEvent.hpp"
#include "Input.hpp"

#include "Bloom/Assets/AssetManager.hpp"

#include <utl/memory.hpp>
#include <utl/vector.hpp>
#include <utl/messenger.hpp>

namespace bloom {
	
	class RenderContext;
	class Renderer;
	class AssetManager;
	class SceneSystem;
	class ScriptEngine;
	
	namespace internal {
		struct AppInternals;
	}
	
	class BLOOM_API Application {
	public:
		Application();
		virtual ~Application();
		
		RenderContext& renderContext() { return *_renderContext; }
		Renderer& renderer() { return *_renderer; }
		AssetManager& assetManager() { return *_assetManager; }
		SceneSystem& sceneSystem() { return *_sceneSystem; }
		ScriptEngine& scriptEngine() { return *_scriptEngine; }
		
		Input const& input() const { return _input; }
		
		TimeStep getUpdateTime() const { return _updateTimer.getTimeStep(); }
		TimeStep getRenderTime() const { return _renderTimer.getTimeStep(); }
		
		utl::listener_id addEventListener(utl::listener);
		void addStaticEventListener(utl::listener);
		void publishEvent(utl::message_type auto const& event) {
			_messenger.send_message(event);
		}
		
		static Application& get() { return *_instance; }
		
	private:
		virtual void init() {}
		virtual void shutdown() {}
		virtual void update(TimeStep) {}
		virtual void render(TimeStep) {}
		virtual void onInputEvent(InputEvent&) {}
		
	private:
		friend struct internal::AppInternals;
		
		void doInit();
		void doShutdown();
		void doUpdate();
		void doRender();
		
		void doTickMainThread();
		
		void handleInputEvent(InputEvent const&);
		
		void flushEventBuffer();
		
		virtual utl::unique_ref<AssetManager> createAssetManager();
		
	private:
		static Application* _instance;
		utl::ref<RenderContext> _renderContext;
		utl::ref<Renderer> _renderer;
		Input _input;
		Timer _renderTimer, _updateTimer;
		utl::ref<AssetManager> _assetManager;
		utl::ref<SceneSystem> _sceneSystem;
		utl::ref<ScriptEngine> _scriptEngine;
		utl::buffered_messenger _messenger;
		utl::listener_id_bag _listenerIDs;
	};

}

bloom::Application* createBloomApplication();

struct bloom::internal::AppInternals {
	static void init(Application*, utl::unique_ref<RenderContext>);
	static void shutdown(Application* app) { app->doShutdown(); }
	static void tick(Application* app) { app->doTickMainThread(); }
	
	static void handleInputEvent(Application* app, InputEvent const& event) { app->handleInputEvent(event); }
};
