#ifndef POPPY_EDITOR_ENTITYSELECTIONMANAGER_H
#define POPPY_EDITOR_ENTITYSELECTIONMANAGER_H

#include <span>

#include "Bloom/Application/MessageSystem.h"
#include "Bloom/Scene/Entity.h"
#include "Poppy/Core/SelectionManager.h"

namespace poppy {

/// Global entity selection manager. This is owned by the editor and shared
/// between views
class EntitySelectionManager:
    public BasicSelectionManager<bloom::EntityHandle>,
    private bloom::Receiver {
public:
    explicit EntitySelectionManager(bloom::Receiver receiver);

    ///  \Returns a view over the selected entities
    std::span<bloom::EntityHandle const> entities() const { return elements(); }
};

} // namespace poppy

#endif // POPPY_EDITOR_ENTITYSELECTIONMANAGER_H
