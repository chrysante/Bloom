#include "Bloom/Application/CoreSystemManager.h"

#include "Bloom/Application/Application.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Renderer/Renderer.h"
#include "Bloom/Runtime/CoreRuntime.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Runtime/ScriptSystem.h"

using namespace bloom;

CoreSystemManager::CoreSystemManager(Application* app): mApp(app) {}

CoreSystemManager::~CoreSystemManager() = default;

void CoreSystemManager::init() {
    mDevice = HardwareDevice::create(RenderAPI::Metal);
    mRenderer = createForwardRenderer(*mApp);
    mRenderer->init(device());
    mAssetManager = makeCoreSystem<AssetManager>();
    mRuntime = makeCoreSystem<CoreRuntime>();
    mSceneSystem = makeCoreSystem<SceneSystem>();
    mScriptSystem = makeCoreSystem<ScriptSystem>();
    mScriptSystem->init();
    mRuntime->setDelegate(mSceneSystem);
}

void CoreSystemManager::shutdown() {}

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
    system->Receiver::operator=(mApp->makeReceiver());
    system->mApp = mApp;
    return system;
}
