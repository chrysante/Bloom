#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"
#include "Event.hpp"
#include "Input.hpp"

#include "Bloom/Assets/AssetManager.hpp"

#include <utl/memory.hpp>
#include <utl/vector.hpp>

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
		virtual ~Application();
		
		RenderContext& renderContext() { return *_renderContext; }
		Renderer& renderer() { return *_renderer; }
		AssetManager& assetManager() { return *_assetManager; }
		SceneSystem& sceneSystem() { return *_sceneSystem; }
		ScriptEngine& scriptEngine() { return *_scriptEngine; }
		
		Input const& input() const { return _input; }
		
		TimeStep getUpdateTime() const { return _updateTimer.getTimeStep(); }
		TimeStep getRenderTime() const { return _renderTimer.getTimeStep(); }
		
	private:
		virtual void init() {}
		virtual void shutdown() {}
		virtual void update(TimeStep) {}
		virtual void render(TimeStep) {}
		virtual void onEvent(Event&) {}
		
		virtual void mouseEventExtra(void* nativeEvent) {}
		
	private:
		friend struct internal::AppInternals;
		
		void doInit();
		void doShutdown();
		void doUpdate();
		void doRender();
		
		void doTickMainThread();
		
		void handleEvent(Event const&);
		
		void flushEventBuffer();
		
		virtual utl::unique_ref<AssetManager> createAssetManager();
		
	private:
		utl::ref<RenderContext> _renderContext;
		utl::ref<Renderer> _renderer;
		Input _input;
		Timer _renderTimer, _updateTimer;
		utl::vector<Event> _eventBuffer;
		utl::ref<AssetManager> _assetManager;
		utl::ref<SceneSystem> _sceneSystem;
		utl::ref<ScriptEngine> _scriptEngine;
	};

}

bloom::Application* createBloomApplication();

struct bloom::internal::AppInternals {
	static void init(Application*, utl::unique_ref<RenderContext>);
	static void shutdown(Application* app) { app->doShutdown(); }
	static void tick(Application* app) { app->doTickMainThread(); }
	
	static void handleEvent(Application* app, Event const& event) { app->handleEvent(event); }
//	static void handleKeyEvent(Application* app, KeyEvent const& event, bool down) { app->handleKeyEvent(event, down); }
};
