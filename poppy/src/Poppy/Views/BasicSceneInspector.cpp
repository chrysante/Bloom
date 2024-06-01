#include "Poppy/Views/BasicSceneInspector.h"

#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/View.h"

using namespace bloom;
using namespace poppy;

template <typename... T>
static std::optional<ComponentKind> getAnyOfSubset(TypeList<T...>,
                                                   EntityHandle entity) {
    std::optional<ComponentKind> result;
    (void)((entity.has<T>() ? ((result = DynamicType<T>), true) : false) ||
           ...);
    return result;
}

std::optional<ComponentKind> poppy::getLightType(EntityHandle entity) {
    return getAnyOfSubset(LightComponentList(), entity);
}

std::optional<ComponentKind> poppy::getPhysicsType(EntityHandle entity) {
    return getAnyOfSubset(MovabilityList(), entity);
}

std::optional<ComponentKind> poppy::getColliderType(EntityHandle entity) {
    return getAnyOfSubset(ColliderList(), entity);
}

Scene* BasicSceneInspector::scene() const {
    return view->editor().sceneSystem().scene();
}

EntitySelectionManager& BasicSceneInspector::selection() const {
    return view->editor().selection();
}

AssetManager& BasicSceneInspector::assetManager() const {
    return view->editor().assetManager();
}

bool BasicSceneInspector::isSimulating() const {
    return view->editor().isSimulating();
}
