#include "Bloom/Core/Debug.h"

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

void Logger::beginLog(Level level) {
    auto& str = ostream();
    if (!tfmt::isTermFormattable(str)) {
        announceMessage(str, level);
    }
    tfmt::pushModifier(mod(level), str);
}

void Logger::endLog() { tfmt::popModifier(ostream()); }

std::ostream& Logger::ostream() { return std::cout; }
