#ifndef BLOOM_CORE_DEBUG_H
#define BLOOM_CORE_DEBUG_H

#include "Bloom/Core/Base.h"

#ifdef BLOOM_CPP

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ostream>
#include <string_view>

#include <utl/common.hpp>
#include <utl/function_view.hpp>

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

[[noreturn]]
#if defined(__GNUC__)
__attribute__((always_inline, nodebug))
#elif defined(_MSC_VER)
__forceinline
#endif
inline void debugfail() {
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
    enum class Level {
        Trace,
        Info,
        Debug,
        Warning,
        Error,
        Fatal,
        LAST = Fatal
    };

    /// Functions for different severity levels @{
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
    /// @}

    /// Customization point
    struct Impl {
        virtual ~Impl() = default;
        virtual void beginLog(Level level) = 0;
        virtual void endLog() = 0;
        virtual std::ostream& getOstream() = 0;
    };

    /// Executes \p callback under the global logger lock
    static void doLocked(utl::function_view<void()> callback);

private:
    struct LogGuard {
        ~LogGuard() { Logger::endLog(); }
    };

    static void doLog(Level level, auto const&... args) noexcept {
        beginLog(level);
        LogGuard guard;
        (..., (defOstream() << args));
        (..., (userOstream() << args));
    }

    static void beginLog(Level level);

    static void endLog();

    static std::ostream& defOstream();
    static std::ostream& userOstream();
};

/// Users can replace this function to return their own logger implementation
__attribute__((weak)) std::unique_ptr<Logger::Impl> makeGlobalLoggerImpl();

#define BL_PROFILE_FUNCTION()                                                  \
    FunctionProfiler UTL_UNIQUE_NAME(BL_Profiler_)(__func__, __FILE__)

///
class FunctionProfiler {
public:
    FunctionProfiler(char const* function, char const* file);

    FunctionProfiler(FunctionProfiler const&) = delete;

    ~FunctionProfiler();

private:
    char const* function;
    char const* file;
    std::chrono::high_resolution_clock::time_point start;
};

} // namespace bloom

#endif // BLOOM_CPP

#endif // BLOOM_CORE_DEBUG_H
