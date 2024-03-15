#ifndef BLOOM_SCENE_COMPONENTS_TAG_H
#define BLOOM_SCENE_COMPONENTS_TAG_H

#include <string>

#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct BLOOM_API TagComponent {
    BLOOM_REGISTER_COMPONENT("Tag")

    std::string name;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::TagComponent, name);

#endif

#endif // BLOOM_SCENE_COMPONENTS_TAG_H
