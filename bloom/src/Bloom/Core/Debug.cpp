#include "Bloom/Core/Debug.h"

#include <iostream>

#include <termfmt/termfmt.h>

using namespace bloom;

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

void Logger::doLogImpl(Level level, std::string_view msg) {
    auto& str = std::cout;
    if (!tfmt::isTermFormattable(str)) {
        announceMessage(str, level);
    }
    tfmt::FormatGuard guard(mod(level), str);
    str << msg << std::endl;
}
