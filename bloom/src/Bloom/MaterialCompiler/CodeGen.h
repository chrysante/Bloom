#ifndef BLOOM_MATERIALCOMPILER_CODEGEN_H
#define BLOOM_MATERIALCOMPILER_CODEGEN_H

#include <filesystem>
#include <memory>
#include <string>

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;

    virtual std::string run() = 0;

protected:
    CodeGenerator() = default;
};

struct CodeGenOptions {
    std::filesystem::path shaderDir;
};

std::unique_ptr<CodeGenerator> makeCodeGenerator(Context const& ctx,
                                                 MaterialGraph const& graph,
                                                 CodeGenOptions const& options);

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_CODEGEN_H
