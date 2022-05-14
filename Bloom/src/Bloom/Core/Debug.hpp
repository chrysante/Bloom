#pragma once

#include "Base.hpp"

#ifdef BLOOM_CPP

#include <utl/fancy_debug.hpp>
#include <utl/log.hpp>
#include <mutex>

#ifndef BLOOM_DEBUGLEVEL
#define BLOOM_DEBUGLEVEL 1
#endif

#ifndef BLOOM_LOGLEVEL
#define BLOOM_LOGLEVEL 1
#endif

#define bloomAssert(...) \
	UTL_FANCY_ASSERT("Bloom", BLOOM_DEBUGLEVEL, assertion, __VA_ARGS__)
#define bloomExpect(...) \
	UTL_FANCY_ASSERT("Bloom", BLOOM_DEBUGLEVEL, precondition, __VA_ARGS__)
#define bloomEnsure(...) \
	UTL_FANCY_ASSERT("Bloom", BLOOM_DEBUGLEVEL, postcondition, __VA_ARGS__)
#define bloomDebugbreak(msg) \
	__utl_debugbreak(msg)
#define bloomDebugfail(msg) \
	__utl_debugfail(msg)
#define bloomBoundsCheck(index, lower, upper) \
	UTL_FANCY_BOUNDS_CHECK("Bloom", BLOOM_DEBUGLEVEL, index, lower, upper)

namespace bloom {
	constexpr utl::log_level logLevelMask = BLOOM_LOGLEVEL == 0 ? utl::log_level::warning | utl::log_level::error | utl::log_level::fatal : utl::log_level::all;
	std::pair<std::unique_lock<std::mutex>, utl::vector<utl::log_message>&> globalLog();
	utl::logger& globalLogger();
}

#define bloomLog(...)                                    \
	UTL_MAKE_LOG_MACRO(::bloom::globalLogger(), \
					   ::bloom::logLevelMask,   \
					   __VA_ARGS__)

#endif // BLOOM_CPP
