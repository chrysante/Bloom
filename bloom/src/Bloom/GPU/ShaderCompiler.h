#ifndef BLOOM_GPU_SHADERCOMPILER_H
#define BLOOM_GPU_SHADERCOMPILER_H

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "Bloom/GPU/Fwd.h"

namespace bloom {

class ShaderLibrary {
public:
    virtual ~ShaderLibrary() = default;

    /// Creates a function handle for the function \p name
    virtual ShaderFunctionHandle createFunction(std::string name) = 0;
};

/// Parameters for shader compilation
struct ShaderCompilerOptions {
    bool preprocessIncludes = true;
};

class ShaderCompiler {
public:
    virtual ~ShaderCompiler() = default;

    /// Reads the file contents of \p sourcePath and returns
    /// `compileSource(<contents>)`
    /// If \p sourcepath is a relative path, it will be treated as relative to
    /// `resourceDir()`
    virtual Reference<ShaderLibrary> compileFile(
        std::filesystem::path sourcePath,
        ShaderCompilerOptions const& options = {});

    ///
    virtual Reference<ShaderLibrary> compileSource(
        std::string source, ShaderCompilerOptions const& options = {}) = 0;

protected:
    ShaderCompiler() = default;
};

} // namespace bloom

#endif // BLOOM_GPU_SHADERCOMPILER_H
