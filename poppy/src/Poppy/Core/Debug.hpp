#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

#ifdef BLOOM_CPP

#include <assert.h>

#ifndef POPPY_DEBUGLEVEL
#define POPPY_DEBUGLEVEL BLOOM_DEBUGLEVEL
#endif

#ifndef POPPY_LOGLEVEL
#define POPPY_LOGLEVEL BLOOM_LOGLEVEL
#endif

#define poppyAssert(...) \
    assert(__VA_ARGS__)
#define poppyExpect(...) \
    assert(__VA_ARGS__)
#define poppyEnsure(...) \
    assert(__VA_ARGS__)
#define poppyDebugbreak(msg) \
	__builtin_debugtrap()
#define poppyDebugfail(msg) \
    __builtin_debugtrap()
#define poppyBoundsCheck(index, lower, upper) \
	assert(index >= lower && index < upper)

#define poppyLog(...)

#endif // BLOOM_CPP
