#ifndef BLOOM_RUNTIME_SCENESYSTEM_H
#define BLOOM_RUNTIME_SCENESYSTEM_H

#include <mutex>
#include <span>

#include <utl/function_view.hpp>
#include <utl/hashmap.hpp>
#include <utl/utility.hpp>
#include <utl/uuid.hpp>

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
    ///
    void init();

    /// Loads the scene \p scene into the system
    void loadScene(Reference<Scene> scene);

    /// Unloads the scene with ID \p ID
    void unloadScene(utl::uuid ID);

    /// Unloads all scenes
    void unloadAll();

    ///
    std::unique_lock<std::mutex> lock();

    /// \Returns a view over all loaded scenes
    std::span<Scene* const> scenes() const { return mScenePtrs; }

    /// Computes the transform matrices for all entities based on the hierarchy
    void applyTransformHierarchy();

private:
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
    utl::hashmap<utl::uuid, Reference<Scene>> mSimScenes;
    utl::hashmap<utl::uuid, Reference<Scene>> mScenes;
    utl::small_vector<Scene*> mScenePtrs;
};

struct BLOOM_API UnloadSceneEvent {
    Scene* scene = nullptr;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_SCENESYSTEM_H
