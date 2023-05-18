#include "Bloom/Application/CoreSystemManager.hpp"

#include "Bloom/Application/Application.hpp"
#include "Bloom/Asset/AssetManager.hpp"
#include "Bloom/GPU/HardwareDevice.hpp"
#include "Bloom/Graphics/Renderer/Renderer.hpp"
#include "Bloom/Runtime/CoreRuntime.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"
#include "Bloom/Runtime/ScriptSystem.hpp"

using namespace bloom;

CoreSystemManager::CoreSystemManager(Application* app): mApp(app) {}

CoreSystemManager::~CoreSystemManager() {}

void CoreSystemManager::init() {
    mDevice   = HardwareDevice::create(RenderAPI::metal);
    mRenderer = createForwardRenderer(*mApp);
    mRenderer->init(device());
    mAssetManager = makeCoreSystem<AssetManager>();
    mRuntime      = makeCoreSystem<CoreRuntime>();
    mSceneSystem  = makeCoreSystem<SceneSystem>();
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
    system->Reciever::operator=(mApp->makeReciever());
    system->mApp = mApp;
    return system;
}
