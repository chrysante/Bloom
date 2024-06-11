#include "Bloom/MaterialCompiler/Debug.h"

using namespace bloom;
using namespace matc;

void matc::unreachableImpl(std::string message, char const* function,
                           char const* file, int line) {
    std::stringstream what;
    what << "Hit unreachable code path\n";
    what << "    With message: " << message << "\n";
    what << "    In function:  " << function << "\n";
    what << "    In file:      " << file << "\n";
    what << "    On line:      " << line << "\n";
    throw Exception(std::move(what).str(), std::move(message), function, file,
                    line);
}

void matc::verifyFailedImpl(char const* expr, std::string message,
                            char const* function, char const* file, int line) {
    std::stringstream what;
    what << "Assertion failed: " << expr << "\n";
    what << "    With message: " << message << "\n";
    what << "    In function:  " << function << "\n";
    what << "    In file:      " << file << "\n";
    what << "    On line:      " << line << "\n";
    throw Exception(std::move(what).str(), std::move(message), function, file,
                    line);
}
