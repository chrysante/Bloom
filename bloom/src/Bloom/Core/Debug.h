#ifndef BLOOM_CORE_DEBUG_H
#define BLOOM_CORE_DEBUG_H

#include "Bloom/Core/Base.h"

#ifdef BLOOM_CPP

#include <cassert>
#include <string_view>

#include <utl/strcat.hpp>

#ifndef BLOOM_DEBUGLEVEL
#define BLOOM_DEBUGLEVEL 1
#endif

#ifndef BLOOM_LOGLEVEL
#define BLOOM_LOGLEVEL 1
#endif

#define BL_DEBUGBREAK(msg) __builtin_debugtrap()

#define BL_DEBUGFAIL(msg) __builtin_trap()

#define BL_UNIMPLEMENTED() __builtin_trap()

#define BL_UNREACHABLE() __builtin_trap()

#define BL_EXPECT(cond) assert(cond)

namespace bloom {

/// Static logger that provides several "severity" modes
class BLOOM_API Logger {
public:
    enum class Level { Trace, Info, Debug, Warning, Error, Fatal };

    static void Trace(auto const&... args) { doLog(Level::Trace, args...); }
    static void Info(auto const&... args) { doLog(Level::Info, args...); }
    static void Debug(auto const&... args) { doLog(Level::Debug, args...); }
    static void Warn(auto const&... args) { doLog(Level::Warning, args...); }
    static void Error(auto const&... args) { doLog(Level::Error, args...); }
    static void Fatal(auto const&... args) { doLog(Level::Fatal, args...); }

private:
    static void doLog(Level level, auto const&... args) {
        doLogImpl(level, utl::strcat(args...));
    }

    static void doLogImpl(Level level, std::string_view msg);
};

} // namespace bloom

#endif // BLOOM_CPP

#endif // BLOOM_CORE_DEBUG_H
