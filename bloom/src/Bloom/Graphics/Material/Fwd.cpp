#include "Bloom/Graphics/Material/Fwd.h"

#include "Bloom/Core/Debug.h"

using namespace bloom;

size_t bloom::layoutSize(MaterialDataType type) {
    using enum MaterialDataType;
    switch (type) {
    case Float:
        return 4;
    case Float2:
        return 8;
    case Float3:
        return 16;
    case Float4:
        return 16;
    default:
        BL_UNREACHABLE();
    }
}
