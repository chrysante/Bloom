#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

#ifdef BLOOM_CPP

#include <utl/fancy_debug.hpp>
#include <utl/log.hpp>
#include <mutex>

#ifndef POPPY_DEBUGLEVEL
#define POPPY_DEBUGLEVEL BLOOM_DEBUGLEVEL
#endif

#ifndef POPPY_LOGLEVEL
#define POPPY_LOGLEVEL BLOOM_LOGLEVEL
#endif

#define poppyAssert(...) \
	UTL_FANCY_ASSERT("Bloom", POPPY_DEBUGLEVEL, assertion, __VA_ARGS__)
#define poppyExpect(...) \
	UTL_FANCY_ASSERT("Bloom", POPPY_DEBUGLEVEL, precondition, __VA_ARGS__)
#define poppyEnsure(...) \
	UTL_FANCY_ASSERT("Bloom", POPPY_DEBUGLEVEL, postcondition, __VA_ARGS__)
#define poppyDebugbreak(msg) \
	__utl_debugbreak(msg)
#define poppyBoundsCheck(index, lower, upper) \
	UTL_FANCY_BOUNDS_CHECK("Bloom", POPPY_DEBUGLEVEL, index, lower, upper)

namespace poppy {
	constexpr utl::log_level logLevelMask = POPPY_LOGLEVEL == 0 ? utl::log_level::warning | utl::log_level::error | utl::log_level::fatal : utl::log_level::all;
	std::pair<std::unique_lock<std::mutex>, utl::vector<utl::log_message>&> globalLog();
	utl::logger& globalLogger();
}

#define poppyLog(...)                                    \
	UTL_MAKE_LOG_MACRO(::poppy::globalLogger(), \
					   ::poppy::logLevelMask,   \
					   __VA_ARGS__)

#endif // BLOOM_CPP
