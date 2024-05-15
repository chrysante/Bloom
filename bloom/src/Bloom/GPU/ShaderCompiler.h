#ifndef BLOOM_GPU_SHADERCOMPILER_H
#define BLOOM_GPU_SHADERCOMPILER_H

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "Bloom/GPU/Fwd.h"

namespace bloom {

class ShaderCompiler {
public:
    virtual ~ShaderCompiler() = default;

    std::vector<ShaderFunctionHandle> compile(
        std::filesystem::path sourcepath,
        std::span<std::string const> functionNames);

    virtual std::vector<ShaderFunctionHandle> compile(
        std::string source, std::span<std::string const> functionNames) = 0;

protected:
    ShaderCompiler() = default;
};

} // namespace bloom

#endif // BLOOM_GPU_SHADERCOMPILER_H
