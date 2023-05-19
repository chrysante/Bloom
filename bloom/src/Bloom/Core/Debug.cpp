#include "Bloom/Core/Debug.hpp"

#include <iostream>

using namespace bloom;

void Logger::logImpl(Level level, std::string_view msg) {
    auto& str = std::cout;
    switch (level) {
        using enum Logger::Level;
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
    str << msg << std::endl;
}
