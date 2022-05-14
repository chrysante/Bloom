#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"
#include "Bloom/Core/Reference.hpp"

#include <thread>
#include <mutex>
#include <chrono>

namespace bloom {

	class Scene;
	class Renderer;
	
	struct UpdateOptions {
		std::size_t stepsPerSecond = 50;
	};
	
    class BLOOM_API SceneSystem {
    public:
		SceneSystem();
		~SceneSystem();
		
		void setScene(Reference<Scene> scene);
		Scene* getScene() { return _scene.get(); }
		
		void setUpdateOptions(UpdateOptions options) { _updateOptions = options; }
		UpdateOptions getUpdateOptions() const { return _updateOptions; }
		
		void runUpdateThread();
		void stopUpdateThread();
		
		void sumbitToRenderer(Renderer&);
		
		void applyTransformHierarchy();
		
		bool isRunning() const { return _isRunning; }
		
		std::unique_lock<std::mutex> lock() { return std::unique_lock<std::mutex>{ _updateMutex }; }
		
		Scene copyScene();
		
	private:
		void updateLoop();
		void calculateTransforms();
		void simulationStep(TimeStep);
		
		
		
    private:
		Reference<Scene> _scene;
		std::thread _updateThread;
		std::mutex _updateMutex;
		std::atomic_bool _isRunning = false;
		std::chrono::high_resolution_clock::time_point _beginTime;
		UpdateOptions _updateOptions{};
    };

}
