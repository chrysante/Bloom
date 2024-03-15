#ifndef BLOOM_SCENE_COMPONENTS_HIERARCHY_H
#define BLOOM_SCENE_COMPONENTS_HIERARCHY_H

#include "Bloom/Scene/Components/ComponentBase.h"
#include "Bloom/Scene/Entity.h"

namespace bloom {

struct BLOOM_API HierarchyComponent {
    BLOOM_REGISTER_COMPONENT("Hierarchy")

    EntityID parent, prevSibling, nextSibling, firstChild, lastChild;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::HierarchyComponent, parent, prevSibling,
                           nextSibling, firstChild, lastChild);

#endif

#endif // BLOOM_SCENE_COMPONENTS_HIERARCHY_H
