#pragma once

#include <utl/strcat.hpp>

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

namespace poppy {

class Logger {
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

} // namespace poppy
