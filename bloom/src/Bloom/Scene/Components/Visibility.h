#ifndef BLOOM_SCENE_COMPONENTS_VISIBILITY_H
#define BLOOM_SCENE_COMPONENTS_VISIBILITY_H

#include "Bloom/Core/Yaml.h"

namespace bloom {

struct VisibilityComponent {
    BLOOM_REGISTER_COMPONENT("Visibility")

    bool value = true;
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::VisibilityComponent, value);

#endif // BLOOM_SCENE_COMPONENTS_VISIBILITY_H
