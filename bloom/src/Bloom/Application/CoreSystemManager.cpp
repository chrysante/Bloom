#include "CoreSystemManager.hpp"

#include "Application.hpp"

#include "Bloom/GPU/HardwareDevice.hpp"

#include "Bloom/Graphics/Renderer/Renderer.hpp"
#include "Bloom/Asset/AssetManager.hpp"
//#include "Bloom/ScriptEngine/ScriptEngine.hpp"

#include "Bloom/Runtime/ScriptSystem.hpp"

#include "Bloom/Runtime/CoreRuntime.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"

namespace bloom {
	
	CoreSystemManager::CoreSystemManager(Application* app): mApp(app) {
		
	}
	
	CoreSystemManager::~CoreSystemManager() {
		
	}
	
	void CoreSystemManager::init() {
		mDevice       = HardwareDevice::create(RenderAPI::metal);
		mRenderer     = createForwardRenderer(*mApp);
		mRenderer->init(device());
		
		// AssetManager
		mAssetManager = makeCoreSystem<AssetManager>();
		
		// ScriptEngine
//		mScriptEngine = makeCoreSystem<ScriptEngine>();
		
		// ScriptSystem
		//mScriptSystem = makeCoreSystem<ScriptSystem>(scriptEngine());
		
		// Runtime
		mRuntime      = makeCoreSystem<CoreRuntime>();
		
		// SceneSystem
		mSceneSystem  = makeCoreSystem<SceneSystem>();
		mRuntime->setDelegate(mSceneSystem);
	}
	
	void CoreSystemManager::shutdown() {
		
	}
	
	std::unique_ptr<Renderer> CoreSystemManager::getRenderer() {
		return std::move(mRenderer);
	}
	
	void CoreSystemManager::setRenderer(std::unique_ptr<Renderer> renderer) {
		mRenderer = std::move(renderer);
	}
	
	
	template <typename SystemType>
	std::unique_ptr<SystemType> CoreSystemManager::makeCoreSystem(auto&&... args) {
		auto system = std::make_unique<SystemType>(UTL_FORWARD(args)...);
		system->Emitter::operator=(mApp->makeEmitter());
		system->Reciever::operator=(mApp->makeReciever());
		system->mApp = mApp;
		return system;
	}
	
}
