#include "Bloom/Core/Debug.h"

#include <fstream>
#include <iomanip>
#include <iostream>

#include <termfmt/termfmt.h>
#include <utl/format_time.hpp>
#include <utl/streammanip.hpp>

using namespace bloom;

void internal::debugErrorMessage(DebugErrorKind kind, int line,
                                 char const* file, char const* function,
                                 char const* expression, char const* message) {
    Logger::Fatal(utl::streammanip([&](std::ostream& str) {
        switch (kind) {
        case Assert:
            str << "Assertion failed: " << expression;
            break;
        case Expect:
            str << "Precondition failed: " << expression;
            break;
        case Unreachable:
            str << "Hit unreachable code path";
            break;
        case Unimplemented:
            str << "Hit unimplemented code path";
            break;
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
        /* Trace:   */ None,
        /* Info:    */ BrightGreen,
        /* Debug:   */ BrightBlue,
        /* Warning: */ Yellow | Bold,
        /* Error:   */ Red | Bold,
        /* Fatal:   */ BGRed | BrightWhite | Bold,
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

namespace {

struct TerminalLogger: Logger::Impl {
    void beginLog(Logger::Level level) final {
        tfmt::pushModifier(mod(level), std::cout);
        if (options.timestamps) {
            formatTime(std::cout);
        }
        if (!tfmt::isTermFormattable(std::cout)) {
            announceMessage(std::cout, level);
        }
    }

    void endLog() final {
        tfmt::popModifier(std::cout);
        std::cout << "\n";
    }

    std::ostream& getOstream() final { return std::cout; }
};

struct NullLogger: Logger::Impl {
    void beginLog(Logger::Level) final {}
    void endLog() final {}
    std::ostream& getOstream() final { return f; }
    std::fstream f;
};

} // namespace

static std::mutex gLoggerMutex;

TerminalLogger& termLogger() {
    static TerminalLogger l;
    return l;
}

static Logger::Impl& userLogger() {
    static auto const l = makeGlobalLoggerImpl();
    return *l;
}

void Logger::doLocked(utl::function_view<void()> callback) {
    std::lock_guard lock(gLoggerMutex);
    callback();
}

void Logger::beginLog(Level level) {
    /// We use RAII to invoke `endLog()` which unlocks the mutex so no need to
    /// use `lock_guard` or `unique_lock` here.
    /// We need the lock because
    ///  - this way messages from different threads won't be interleaved
    ///  - otherwise pushing and popping `tfmt` modifiers from different threads
    ///    does not work correctly
    gLoggerMutex.lock();
    termLogger().beginLog(level);
    userLogger().beginLog(level);
}

void Logger::endLog() {
    termLogger().endLog();
    userLogger().endLog();
    gLoggerMutex.unlock();
}

std::ostream& Logger::defOstream() { return termLogger().getOstream(); }

std::ostream& Logger::userOstream() { return userLogger().getOstream(); }

std::unique_ptr<Logger::Impl> bloom::makeGlobalLoggerImpl() {
    return std::make_unique<NullLogger>();
}

FunctionProfiler::FunctionProfiler(char const* function, char const* file):
    function(function),
    file(file),
    start(std::chrono::high_resolution_clock::now()) {}

FunctionProfiler::~FunctionProfiler() {
    (void)file;
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    Logger::Trace(function, " : ", utl::format_duration(elapsed));
}
