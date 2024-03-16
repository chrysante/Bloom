#ifndef BLOOM_CORE_DEBUG_H
#define BLOOM_CORE_DEBUG_H

#include "Bloom/Core/Base.h"

#ifdef BLOOM_CPP

#include <cassert>
#include <cstdlib>
#include <ostream>
#include <string_view>

#include <utl/common.hpp>

#ifndef BLOOM_DEBUGLEVEL
#define BLOOM_DEBUGLEVEL 1
#endif

#ifndef BLOOM_LOGLEVEL
#define BLOOM_LOGLEVEL 1
#endif

/// Use this macro to mark unimplemented code paths
#define BL_UNIMPLEMENTED()                                                     \
    (::bloom::internal::debugErrorMessage(::bloom::internal::Unimplemented,    \
                                          __LINE__, __FILE__,                  \
                                          __PRETTY_FUNCTION__, nullptr,        \
                                          nullptr),                            \
     ::bloom::internal::debugfail())

/// Use this macro to mark unreachable code paths
#define BL_UNREACHABLE()                                                       \
    (::bloom::internal::debugErrorMessage(::bloom::internal::Unreachable,      \
                                          __LINE__, __FILE__,                  \
                                          __PRETTY_FUNCTION__, nullptr,        \
                                          nullptr),                            \
     ::bloom::internal::debugfail())

#define BL_IMPL_ASSERT_2(kind, cond)                                           \
    (cond ? (void)0 :                                                          \
            (::bloom::internal::debugErrorMessage(::bloom::internal::kind,     \
                                                  __LINE__, __FILE__,          \
                                                  __PRETTY_FUNCTION__, #cond,  \
                                                  nullptr),                    \
             ::bloom::internal::debugfail()))

#define BL_IMPL_ASSERT_3(kind, cond, message)                                  \
    (cond ? (void)0 :                                                          \
            (::bloom::internal::debugErrorMessage(::bloom::internal::kind,     \
                                                  __LINE__, __FILE__,          \
                                                  __PRETTY_FUNCTION__, #cond,  \
                                                  message),                    \
             ::bloom::internal::debugfail()))

///
#define BL_ASSERT(...) UTL_VFUNC(BL_IMPL_ASSERT_, Assert, __VA_ARGS__)

///
#define BL_EXPECT(...) UTL_VFUNC(BL_IMPL_ASSERT_, Expect, __VA_ARGS__)

namespace bloom::internal {

enum DebugErrorKind { Assert, Expect, Unreachable, Unimplemented };

void debugErrorMessage(DebugErrorKind kind, int line, char const* file,
                       char const* function, char const* expression,
                       char const* message);

enum AssertionHandler { Break, Throw, Abort };

// For now
inline AssertionHandler getAssertionHandler() { return Break; }

[[noreturn]] __attribute__((always_inline)) inline void debugfail() {
    switch (getAssertionHandler()) {
    case Break:
        __builtin_trap();
    case Throw:
        throw std::runtime_error("Assertion failed");
    case Abort:
        std::abort();
    }
}

} // namespace bloom::internal

namespace bloom {

/// Static logger that provides several "severity" modes
class BLOOM_API Logger {
public:
    enum class Level { Trace, Info, Debug, Warning, Error, Fatal };

    static void Trace(auto const&... args) noexcept {
        doLog(Level::Trace, args...);
    }
    static void Info(auto const&... args) noexcept {
        doLog(Level::Info, args...);
    }
    static void Debug(auto const&... args) noexcept {
        doLog(Level::Debug, args...);
    }
    static void Warn(auto const&... args) noexcept {
        doLog(Level::Warning, args...);
    }
    static void Error(auto const&... args) noexcept {
        doLog(Level::Error, args...);
    }
    static void Fatal(auto const&... args) noexcept {
        doLog(Level::Fatal, args...);
    }

private:
    static void doLog(Level level, auto const&... args) noexcept {
        beginLog(level);
        (..., (ostream() << args));
        endLog();
    }

    static void beginLog(Level level);

    static void endLog();

    static std::ostream& ostream();
};

} // namespace bloom

#endif // BLOOM_CPP

#endif // BLOOM_CORE_DEBUG_H
