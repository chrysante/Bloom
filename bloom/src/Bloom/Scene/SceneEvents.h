#ifndef BLOOM_SCENE_SCENEEVENTS_H
#define BLOOM_SCENE_SCENEEVENTS_H

#include "Bloom/Core/Base.h"

namespace bloom {

class Scene;

/// Sent when scenes are loaded into the scene system
struct BLOOM_API SceneLoadedEvent {
    Scene* scene;
};

} // namespace bloom

#endif // BLOOM_SCENE_SCENEEVENTS_H
