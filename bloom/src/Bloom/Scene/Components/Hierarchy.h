#ifndef BLOOM_SCENE_COMPONENTS_HIERARCHY_H
#define BLOOM_SCENE_COMPONENTS_HIERARCHY_H

#include "Bloom/Scene/Components/ComponentBase.h"
#include "Bloom/Scene/EntityID.h"

namespace bloom {

struct HierarchyComponent {
    BLOOM_REGISTER_COMPONENT("Hierarchy")

    EntityID parent, prevSibling, nextSibling, firstChild, lastChild;
};

} // namespace bloom

#include "Bloom/Core/Yaml.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::HierarchyComponent, parent, prevSibling,
                           nextSibling, firstChild, lastChild);

#endif // BLOOM_SCENE_COMPONENTS_HIERARCHY_H
