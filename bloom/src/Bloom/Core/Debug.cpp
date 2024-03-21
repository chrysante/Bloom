#include "Bloom/Core/Debug.h"

#include <iomanip>
#include <iostream>

#include <termfmt/termfmt.h>
#include <utl/streammanip.hpp>

using namespace bloom;

void internal::debugErrorMessage(DebugErrorKind kind, int line,
                                 char const* file, char const* function,
                                 char const* expression, char const* message) {
    Logger::Fatal(utl::streammanip([&](std::ostream& str) {
        switch (kind) {
        case Assert:
            str << "Assertion failed: " << expression;
        case Expect:
            str << "Precondition failed: " << expression;
        case Unreachable:
            str << "Hit unreachable code path";
        case Unimplemented:
            str << "Hit unimplemented code path";
        }
        if (message) {
            str << " with message: " << message;
        }
        str << "\nOn line " << line << " in function " << function
            << " in file " << file;
    }));
}

static void announceMessage(std::ostream& str, Logger::Level level) {
    using enum Logger::Level;
    switch (level) {
    case Trace:
        break;
    case Info:
        str << "Info: ";
        break;
    case Debug:
        str << "Debug: ";
        break;
    case Warning:
        str << "Warning: ";
        break;
    case Error:
        str << "Error: ";
        break;
    case Fatal:
        str << "Fatal: ";
        break;
    }
}

static tfmt::Modifier const& mod(Logger::Level level) {
    using namespace tfmt::modifiers;
    static tfmt::Modifier const mods[] = {
        /* Trace   = */ None,
        /* Info    = */ BrightGreen,
        /* Debug   = */ BrightBlue,
        /* Warning = */ Yellow | Bold,
        /* Error   = */ Red | Bold,
        /* Fatal   = */ BGRed | BrightWhite | Bold,
    };
    return mods[(size_t)level];
};

namespace {

struct LoggerOptions {
    bool timestamps;
};

LoggerOptions const options = [] {
    /// TODO: Implement logger control through environment variables
    auto* values = std::getenv("BLOOM_LOGGER_OPTIONS");
    (void)values;
    return LoggerOptions{ .timestamps = true };
}();

} // namespace

static void formatTime(std::ostream& str) {
    // FIXME: This does not compute current daytime correctly
    using namespace std::chrono;
    auto now = high_resolution_clock::now();
    auto dur = now.time_since_epoch();
    dur -= duration_cast<days>(dur);
    auto h = duration_cast<hours>(dur);
    dur -= h;
    auto min = duration_cast<minutes>(dur);
    dur -= min;
    auto sec = duration_cast<seconds>(dur);
    dur -= sec;
    auto mil = duration_cast<milliseconds>(dur);
    dur -= mil;
    str << "[" << std::setfill(' ') << std::setw(2) << h.count()
        << "h:" << std::setfill(' ') << std::setw(2) << min.count()
        << "m:" << std::setfill(' ') << std::setw(2) << sec.count()
        << "s:" << std::setfill(' ') << std::setw(3) << mil.count() << "ms] ";
}

static std::mutex gLoggerMutex;

void Logger::beginLog(Level level) {
    /// We use RAII to invoke `endLog()` which unlocks the mutex so no need to
    /// use `lock_guard` or `unique_lock` here.
    /// We need the lock because
    ///  - this way messages from different threads won't be interleaved
    ///  - Otherwise pushing and popping `tfmt` modifiers from different threads
    ///  does not work correctly
    gLoggerMutex.lock();
    auto& str = ostream();
    tfmt::pushModifier(mod(level), str);
    if (options.timestamps) {
        formatTime(str);
    }
    if (!tfmt::isTermFormattable(str)) {
        announceMessage(str, level);
    }
}

void Logger::endLog() {
    tfmt::popModifier(ostream());
    ostream() << "\n";
    gLoggerMutex.unlock();
}

std::ostream& Logger::ostream() { return std::cout; }
