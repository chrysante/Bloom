#include "Bloom/Core/Autorelease.h"

namespace bloom {

void autoreleased(utl::function<void()> const& block) {
    @autoreleasepool {
        block();
    }
}

} // namespace bloom
