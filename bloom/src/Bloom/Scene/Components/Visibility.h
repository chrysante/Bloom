#ifndef BLOOM_SCENE_COMPONENTS_VISIBILITY_H
#define BLOOM_SCENE_COMPONENTS_VISIBILITY_H

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Yaml.h"

namespace bloom {

struct BLOOM_API VisibilityComponent {
    bool value = true;
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::VisibilityComponent, value);

#endif // BLOOM_SCENE_COMPONENTS_VISIBILITY_H
