#include "Bloom/Application/CoreSystemManager.h"

#include "Bloom/Application/Application.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Runtime/CoreRuntime.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Runtime/ScriptSystem.h"

using namespace bloom;

CoreSystemManager::CoreSystemManager(Application* app): mApp(app) {}

CoreSystemManager::~CoreSystemManager() = default;

void CoreSystemManager::init() {
    mDevice = HardwareDevice::create(RenderAPI::Metal);
    mCommandQueue = mDevice->createCommandQueue();
    mAssetManager = makeCoreSystem<AssetManager>();
    mRuntime = makeCoreSystem<CoreRuntime>();
    mSceneSystem = makeCoreSystem<SceneSystem>();
    mScriptSystem = makeCoreSystem<ScriptSystem>();
    mRuntime->addDelegate(mSceneSystem);
}

void CoreSystemManager::shutdown() {}

template <typename SystemType>
std::unique_ptr<SystemType> CoreSystemManager::makeCoreSystem(auto&&... args) {
    auto system = std::make_unique<SystemType>(UTL_FORWARD(args)...);
    system->assignEmitter(mApp->makeEmitter());
    system->assignReceiver(mApp->makeReceiver());
    system->mApp = mApp;
    system->init();
    return system;
}
