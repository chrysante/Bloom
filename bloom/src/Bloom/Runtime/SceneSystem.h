#ifndef BLOOM_RUNTIME_SCENESYSTEM_H
#define BLOOM_RUNTIME_SCENESYSTEM_H

#include <mutex>
#include <span>

#include <utl/hashmap.hpp>
#include <utl/utility.hpp>
#include <utl/uuid.hpp>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Runtime/CoreRuntime.h"

namespace bloom {

class Scene;

class BLOOM_API SceneSystem: public CoreSystem, public RuntimeDelegate {
public:
    void loadScene(Reference<Scene>);

    void unloadScene(utl::uuid id);

    void unloadAll();

    std::unique_lock<std::mutex> lock();

    std::span<Scene* const> scenes() const { return mScenePtrs; }

    void applyTransformHierarchy();

private:
    void start() override;

    void stop() override;

    void pause() override;

    void resume() override;

    void step(Timestep) override;

    void tryCopyOut();

    void setPointers();

private:
    std::mutex mMutex;
    utl::hashmap<utl::uuid, Reference<Scene>> mBackupScenes;
    utl::hashmap<utl::uuid, Reference<Scene>> mSimScenes;
    utl::hashmap<utl::uuid, Reference<Scene>> mScenes;
    utl::vector<Scene*> mScenePtrs;
};

struct BLOOM_API UnloadSceneEvent {
    Scene* scene = nullptr;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_SCENESYSTEM_H
