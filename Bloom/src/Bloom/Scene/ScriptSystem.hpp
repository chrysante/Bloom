#pragma once

#include "Bloom/Core/Time.hpp"
#include "Bloom/ScriptEngine/ScriptEngine.hpp"

namespace bloom {

	class SceneSystem;
	
    class ScriptSystem {
    public:
		ScriptSystem(SceneSystem*);
		void init();
		
		void onSceneConstruction();
		void onSceneInit();
		void onSceneUpdate(TimeStep);
		void onSceneRender();
		
	private:
		void onScriptReload();
		
		void forEach(auto&& fn);
		
    private:
		SceneSystem* sceneSystem = nullptr;
		std::function<void(ScriptObject&)> initFn;
		std::function<void(ScriptObject&, TimeStep)> updateFn;
		std::function<void(ScriptObject&)> renderFn;
    };

}
