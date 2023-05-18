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

#define BL_DEBUGBREAK(msg) __builtin_debugtrap()
#define BL_DEBUGFAIL(msg)  __builtin_trap()

#define BL_LOG(...)

#endif // BLOOM_CPP
