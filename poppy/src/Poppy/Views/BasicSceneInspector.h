#ifndef POPPY_VIEWS_BASICSCENEINSPECTOR_H
#define POPPY_VIEWS_BASICSCENEINSPECTOR_H

#include <optional>
#include <span>
#include <string>

#include <utl/vector.hpp>

#include "Bloom/Graphics/Lights.h"
#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/PhysicsComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Poppy/Core/Debug.h"

namespace bloom {

class Scene;
class AssetManager;
class Application;

} // namespace bloom

namespace poppy {

class Editor;
class View;
class EntitySelectionManager;

std::optional<bloom::ComponentKind> getPhysicsType(bloom::EntityHandle entity);
std::optional<bloom::ComponentKind> getColliderType(bloom::EntityHandle entity);
std::optional<bloom::ComponentKind> getLightType(bloom::EntityHandle entity);

class BasicSceneInspector {
protected:
    BasicSceneInspector(View* view): view(view) {}

public:
    bloom::Scene* scene() const;

    EntitySelectionManager& selection() const;

    bloom::AssetManager& assetManager() const;

    bool isSimulating() const;

private:
    View* view = nullptr;
};

} // namespace poppy

#endif // POPPY_VIEWS_BASICSCENEINSPECTOR_H
