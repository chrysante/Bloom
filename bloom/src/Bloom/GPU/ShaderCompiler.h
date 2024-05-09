#ifndef BLOOM_GPU_SHADERCOMPILER_H
#define BLOOM_GPU_SHADERCOMPILER_H

#include <string>

#include "Bloom/GPU/Fwd.h"

namespace bloom {

class ShaderCompiler {
public:
    virtual ~ShaderCompiler() = default;

    virtual ShaderFunctionHandle compileFunction(std::string source,
                                                 std::string name) = 0;

protected:
    ShaderCompiler() = default;
};

} // namespace bloom

#endif // BLOOM_GPU_SHADERCOMPILER_H
