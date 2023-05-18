#pragma once

#include "Bloom/Core/Core.hpp"

#include <memory>

namespace bloom {

class Application;

class HardwareDevice;
class Renderer;
class AssetManager;
// class ScriptEngine;
// class ScriptSystem;
class SceneSystem;

class CoreRuntime;
class SceneSystem;
class CoreSystem;

class BLOOM_API CoreSystemManager {
public:
    CoreSystemManager(Application*);
    ~CoreSystemManager();

    void init();
    void shutdown();

    HardwareDevice& device() { return *mDevice; }
    Renderer& renderer() { return *mRenderer; }
    AssetManager& assetManager() { return *mAssetManager; }
    // ScriptEngine&   scriptEngine() { return *mScriptEngine; }

    // ScriptSystem&   scriptSystem() { return *mScriptSystem; }

    CoreRuntime& runtime() { return *mRuntime; }
    SceneSystem& sceneSystem() { return *mSceneSystem; }

    [[nodiscard]] std::unique_ptr<Renderer> getRenderer();
    void setRenderer(std::unique_ptr<Renderer> renderer);

private:
    template <typename SystemType>
    std::unique_ptr<SystemType> makeCoreSystem(auto&&...);

private:
    Application* mApp = nullptr;
    std::unique_ptr<HardwareDevice> mDevice;
    std::unique_ptr<Renderer> mRenderer;
    std::unique_ptr<AssetManager> mAssetManager;
    // std::unique_ptr<ScriptEngine>   mScriptEngine;

    // std::unique_ptr<ScriptSystem>   mScriptSystem;
    std::unique_ptr<CoreRuntime> mRuntime;
    std::shared_ptr<SceneSystem> mSceneSystem;
};

} // namespace bloom
