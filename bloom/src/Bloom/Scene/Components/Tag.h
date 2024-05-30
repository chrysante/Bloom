#ifndef BLOOM_SCENE_COMPONENTS_TAG_H
#define BLOOM_SCENE_COMPONENTS_TAG_H

#include <string>

#include <utl/uuid.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct BLOOM_API TagComponent {
    std::string name;
};

struct BLOOM_API PersistentIDComponent {
    utl::uuid value;
};

} // namespace bloom

#include "Bloom/Core/Yaml.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::TagComponent, name);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PersistentIDComponent, value);

#endif // BLOOM_SCENE_COMPONENTS_TAG_H
