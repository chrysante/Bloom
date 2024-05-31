#ifndef BLOOM_RUNTIME_SCENESYSTEM_H
#define BLOOM_RUNTIME_SCENESYSTEM_H

#include <mutex>
#include <span>

#include <utl/function_view.hpp>
#include <utl/hashmap.hpp>
#include <utl/utility.hpp>
#include <utl/uuid.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Runtime/CoreRuntime.h"

namespace bloom {

class Scene;

/// Manages the runtime behaviour of scenes.
/// Can have multiple scenes loaded at the same time .
class BLOOM_API SceneSystem: public CoreSystem, public RuntimeDelegate {
public:
    /// Loads the scene \p scene into the system.
    /// Unloads the currently loaded scene
    void loadScene(Reference<Scene> scene);

    /// Unloads the currently loaded scene
    void unloadScene();

    ///
    std::unique_lock<std::mutex> lock();

    /// \Returns the loaded editor scene
    Scene* scene() const { return mScene.get(); }

    /// \Returns the scene being run or simulated. This is null if no simulation
    /// is running
    Scene* playScene() const { return mPlayScene.get(); }

private:
    void init() override;

    /// Implementation of `RuntimeDelegate` interface
    /// @{
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void step(Timestep) override;
    /// @}

    ///
    void setPointers(auto const& sceneMap);

private:
    std::mutex mMutex;
    Reference<Scene> mScene;
    Reference<Scene> mPlayScene;
};

struct BLOOM_API UnloadSceneEvent {
    Scene* scene = nullptr;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_SCENESYSTEM_H
