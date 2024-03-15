#ifndef POPPY_CORE_PROFILE_H
#define POPPY_CORE_PROFILE_H

#include <utl/common.hpp>

#include "Bloom/Core/Profile.h"

#define POPPY_PROFILE(STAGE, ...)                                              \
    UTL_VFUNC(POPPY_PROFILE_, STAGE __VA_OPT__(, ) __VA_ARGS__)

#define POPPY_PROFILE_1(STAGE)                                                 \
    BLOOM_PROFILE_IMPL(&::poppy::Profilers::STAGE, __PRETTY_FUNCTION__)
#define POPPY_PROFILE_2(STAGE, NAME)                                           \
    BLOOM_PROFILE_IMPL(&::poppy::Profilers::STAGE, NAME)

namespace poppy {

struct Profilers {
    static bloom::Profile frame;
    static bloom::Profile init;
};

} // namespace poppy

#endif // POPPY_CORE_PROFILE_H