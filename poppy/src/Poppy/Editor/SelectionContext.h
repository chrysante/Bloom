#ifndef POPPY_EDITOR_SELECTIONCONTEXT_H
#define POPPY_EDITOR_SELECTIONCONTEXT_H

#include <span>

#include <utl/utility.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application/MessageSystem.h"
#include "Bloom/Scene/Entity.h"

namespace poppy {

class SelectionContext: bloom::Receiver {
public:
    explicit SelectionContext(bloom::Receiver);

    std::span<bloom::EntityHandle const> entities() const { return mEntities; }

    bool empty() const { return mEntities.empty(); }

    void select(bloom::EntityHandle);
    void addSelect(bloom::EntityHandle);
    void deselect(bloom::EntityHandle);
    void clear();

    bool isSelected(bloom::EntityHandle) const;

private:
    utl::vector<bloom::EntityHandle> mEntities;
};

} // namespace poppy

#endif // POPPY_EDITOR_SELECTIONCONTEXT_H
