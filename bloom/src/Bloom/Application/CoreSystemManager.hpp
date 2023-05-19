#pragma once

#include "Bloom/Core/Core.hpp"

#include <memory>

namespace bloom {

class Application;

class HardwareDevice;
class Renderer;
class AssetManager;
class SceneSystem;
class ScriptSystem;
class CoreRuntime;
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

    CoreRuntime& runtime() { return *mRuntime; }

    SceneSystem& sceneSystem() { return *mSceneSystem; }

    ScriptSystem& scriptSystem() { return *mScriptSystem; }

    [[nodiscard]] std::unique_ptr<Renderer> getRenderer();

    void setRenderer(std::unique_ptr<Renderer> renderer);

private:
    template <typename SystemType>
    std::unique_ptr<SystemType> makeCoreSystem(auto&&...);

    Application* mApp = nullptr;
    std::unique_ptr<HardwareDevice> mDevice;
    std::unique_ptr<Renderer> mRenderer;
    std::unique_ptr<AssetManager> mAssetManager;
    std::unique_ptr<CoreRuntime> mRuntime;
    std::shared_ptr<SceneSystem> mSceneSystem;
    std::shared_ptr<ScriptSystem> mScriptSystem;
};

} // namespace bloom
