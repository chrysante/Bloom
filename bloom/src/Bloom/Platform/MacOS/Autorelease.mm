#include "Bloom/Core/Autorelease.h"

#include "Bloom/Platform/MacOS/ObjCCommon.h"

namespace bloom {

void autoreleased(utl::function<void()> const& block) {
    @autoreleasepool {
        block();
    }
}

} // namespace bloom
