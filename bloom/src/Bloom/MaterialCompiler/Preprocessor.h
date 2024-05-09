#ifndef BLOOM_MATERIALCOMPILER_PREPROCESSOR_H
#define BLOOM_MATERIALCOMPILER_PREPROCESSOR_H

#include <filesystem>
#include <span>
#include <string>

#include "Bloom/Core/Base.h"

namespace bloom {

/// Recursively replaces all `#include` directives with the respective file
/// contents
BLOOM_API std::string preprocessIncludes(
    std::string source, std::span<std::filesystem::path const> searchPaths);

} // namespace bloom

#endif // BLOOM_MATERIALCOMPILER_PREPROCESSOR_H
