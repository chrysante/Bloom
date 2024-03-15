#ifndef BLOOM_APPLICATION_CORESYSTEMMANAGER_H
#define BLOOM_APPLICATION_CORESYSTEMMANAGER_H

#include <memory>

#include "Bloom/Core/Core.h"

namespace bloom {

class Application;

class HardwareDevice;
class Renderer;
class AssetManager;
class SceneSystem;
class ScriptSystem;
class CoreRuntime;
class CoreSystem;

/// "Container" type for all core systems
class BLOOM_API CoreSystemManager {
public:
    /// Construct with reference to the running application
    CoreSystemManager(Application* app);

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

#endif // BLOOM_APPLICATION_CORESYSTEMMANAGER_H
