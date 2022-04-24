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
	
	namespace internal {
		struct AppInternals;
	}
	
	class BLOOM_API Application {
	public:
		virtual ~Application();
		
		RenderContext& renderContext() { return *_renderContext; }
		Renderer& renderer() { return *_renderer; }
		
		Input const& input() const { return _input; }
		
		TimeStep getUpdateTime() const { return _updateTimer.getTimeStep(); }
		TimeStep getRenderTime() const { return _renderTimer.getTimeStep(); }
		
		AssetManager* getAssetManager() { return _assetManager.get(); }
		
	private:
		virtual void init() {}
		virtual void shutdown() {}
		virtual void update(TimeStep) {}
		virtual void render(TimeStep) {}
		virtual void onEvent(Event const&) {}
		
		virtual void mouseEventExtra(void* nativeEvent) {}
		
	private:
		friend struct internal::AppInternals;
		
		void doInit();
		void doShutdown();
		void doUpdate();
		void doRender();
		
		void doTickMainThread();
		
		void handleEvent(Event const&, void* nativeEvent);
		
		void flushEventBuffer();
		
		virtual utl::unique_ref<AssetManager> createAssetManager();
		
	private:
		utl::unique_ref<RenderContext> _renderContext;
		utl::unique_ref<Renderer> _renderer;
		Input _input;
		Timer _updateTimer, _renderTimer;
		utl::vector<Event> _eventBuffer;
		utl::unique_ref<AssetManager> _assetManager;
	};

}

bloom::Application* createBloomApplication();

struct bloom::internal::AppInternals {
	static void init(Application*, utl::unique_ref<RenderContext>);
	static void shutdown(Application* app) { app->doShutdown(); }
	static void tick(Application* app) { app->doTickMainThread(); }
	
	static void handleEvent(Application* app, Event const& event, void* nativeEvent) { app->handleEvent(event, nativeEvent); }
//	static void handleKeyEvent(Application* app, KeyEvent const& event, bool down) { app->handleKeyEvent(event, down); }
};
