#pragma once

#include "CoreRuntime.hpp"

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"
#include "Bloom/Application/CoreSystem.hpp"

#include <mutex>
#include <span>
#include <utl/hashmap.hpp>
#include <utl/UUID.hpp>
#include <utl/utility.hpp>

namespace bloom {
	
	class Scene;
	
	class BLOOM_API SceneSystem: public CoreSystem, public RuntimeDelegate {
	public:
		void loadScene(Reference<Scene>);
		void unloadScene(utl::UUID id);
		void unloadAll();
		std::unique_lock<std::mutex> lock();
		std::span<Scene* const> scenes() const { return mScenePtrs; }
		void applyTransformHierarchy();
		
	private:
		void start() override;
		void stop() override;
		void pause() override;
		void resume() override;
		
		void step(Timestep) override;
		
		void tryCopyOut();
		void setPointers();
		
	private:
//		std::mutex writeMutex, readMutex;
		std::mutex mMutex;
		utl::hashmap<utl::UUID, Reference<Scene>> mBackupScenes;
		utl::hashmap<utl::UUID, Reference<Scene>> mSimScenes;
		utl::hashmap<utl::UUID, Reference<Scene>> mScenes;
		utl::vector<Scene*> mScenePtrs;
	};
	
	struct BLOOM_API UnloadSceneEvent {
		Scene* scene = nullptr;
	};
	
}
