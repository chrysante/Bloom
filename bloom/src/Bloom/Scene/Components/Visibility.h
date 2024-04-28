#ifndef Visibility_h
#define Visibility_h

#include "Bloom/Core/Serialize.h"

namespace bloom {

struct VisibilityComponent {
    BLOOM_REGISTER_COMPONENT("Visibility")

    bool value = true;
};

} // namespace bloom

BLOOM_MAKE_TEXT_SERIALIZER(bloom::VisibilityComponent, value);

#endif /* Visibility_h */
