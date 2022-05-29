#pragma once

#include "Bloom/Core/Time.hpp"
#include "Bloom/Application/CoreSystem.hpp"
#include "Bloom/ScriptEngine/ScriptEngine.hpp"

namespace bloom {
	
    class ScriptSystem: public CoreSystem {
    public:
		ScriptSystem(ScriptEngine&);
		void init();
		
		void onSceneConstruction();
		void onSceneInit();
		void onSceneUpdate(Timestep);
		void onSceneRender();
		
	private:
		void onScriptReload();
		
		void forEach(auto&& fn);
		
    private:
		ScriptEngine* mEngine = nullptr;
		
		std::function<void(ScriptObject&)> initFn;
		std::function<void(ScriptObject&, Timestep)> updateFn;
		std::function<void(ScriptObject&)> renderFn;
    };

}
