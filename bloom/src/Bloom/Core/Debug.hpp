#pragma once

#include "Base.hpp"

#ifdef BLOOM_CPP

#include <assert.h>

#ifndef BLOOM_DEBUGLEVEL
#define BLOOM_DEBUGLEVEL 1
#endif

#ifndef BLOOM_LOGLEVEL
#define BLOOM_LOGLEVEL 1
#endif

#define bloomAssert(...) \
    assert(__VA_ARGS__)
#define bloomExpect(...) \
    assert(__VA_ARGS__)
#define bloomEnsure(...) \
    assert(__VA_ARGS__)
#define bloomDebugbreak(msg) \
	__builtin_debugtrap()
#define bloomDebugfail(msg) \
    __builtin_trap()
#define bloomBoundsCheck(index, lower, upper) \
    assert(index >= lower && index < upper)

#define bloomLog(...)

#endif // BLOOM_CPP
