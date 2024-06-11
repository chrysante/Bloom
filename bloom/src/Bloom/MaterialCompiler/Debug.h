#ifndef BLOOM_MATERIALCOMPILER_DEBUG_H
#define BLOOM_MATERIALCOMPILER_DEBUG_H

#include <stdexcept>

#include <utl/strcat.hpp>

#define UNREACHABLE(...)                                                       \
    bloom::matc::unreachableImpl(utl::strcat(__VA_ARGS__),                     \
                                 __PRETTY_FUNCTION__, __FILE__, __LINE__)

#define VERIFY(condition, ...)                                                 \
    ((condition) ?                                                             \
         (void)0 :                                                             \
         bloom::matc::verifyFailedImpl(#condition, utl::strcat(__VA_ARGS__),   \
                                       __PRETTY_FUNCTION__, __FILE__,          \
                                       __LINE__))

namespace bloom::matc {

class Exception: public std::runtime_error {
public:
    explicit Exception(std::string what, std::string message,
                       std::string function, std::string file, int line):
        std::runtime_error(std::move(what)),
        _message(std::move(message)),
        _function(std::move(function)),
        _file(std::move(file)),
        _line(line) {}

    std::string const& message() const { return _message; }
    std::string const& function() const { return _function; }
    std::string const& file() const { return _file; }
    int line() const { return _line; }

private:
    std::string _message;
    std::string _function;
    std::string _file;
    int _line;
};

[[noreturn]] void unreachableImpl(std::string message, char const* function,
                                  char const* file, int line);

[[noreturn]] void verifyFailedImpl(char const* expr, std::string message,
                                   char const* function, char const* file,
                                   int line);

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_DEBUG_H
