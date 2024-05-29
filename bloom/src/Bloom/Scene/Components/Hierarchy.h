#ifndef BLOOM_SCENE_COMPONENTS_HIERARCHY_H
#define BLOOM_SCENE_COMPONENTS_HIERARCHY_H

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/ComponentBase.h"
#include "Bloom/Scene/EntityID.h"

namespace bloom {

struct BLOOM_API HierarchyComponent {
    EntityID parent, prevSibling, nextSibling, firstChild, lastChild;
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::HierarchyComponent, parent, prevSibling,
                           nextSibling, firstChild, lastChild);

#endif // BLOOM_SCENE_COMPONENTS_HIERARCHY_H
