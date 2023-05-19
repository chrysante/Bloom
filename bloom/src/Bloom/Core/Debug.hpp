#pragma once

#include "Base.hpp"

#ifdef BLOOM_CPP

#include <assert.h>
#include <string_view>

#include <utl/strcat.hpp>

#ifndef BLOOM_DEBUGLEVEL
#define BLOOM_DEBUGLEVEL 1
#endif

#ifndef BLOOM_LOGLEVEL
#define BLOOM_LOGLEVEL 1
#endif

#define BL_DEBUGBREAK(msg) __builtin_debugtrap()
#define BL_DEBUGFAIL(msg)  __builtin_trap()

namespace bloom {

class BLOOM_API Logger {
    enum class Level { Trace, Info, Debug, Warning, Error, Fatal };

public:
    static void trace(auto const&... args) { log(Level::Warning, args...); }
    static void info(auto const&... args) { log(Level::Warning, args...); }
    static void debug(auto const&... args) { log(Level::Warning, args...); }
    static void warn(auto const&... args) { log(Level::Warning, args...); }
    static void error(auto const&... args) { log(Level::Warning, args...); }
    static void fatal(auto const&... args) { log(Level::Warning, args...); }

private:
    static void log(Level level, auto const&... args) {
        logImpl(level, utl::strcat(args...));
    }

    static void logImpl(Level level, std::string_view msg);
};

} // namespace bloom

#endif // BLOOM_CPP
