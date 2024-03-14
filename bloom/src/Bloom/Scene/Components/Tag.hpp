#pragma once

#include <string>

#include "Bloom/Scene/Components/ComponentBase.hpp"

namespace bloom {

struct BLOOM_API TagComponent {
    BLOOM_REGISTER_COMPONENT("Tag")

    std::string name;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::TagComponent, name);

#endif
