#ifndef BLOOM_MATERIALCOMPILER_CODEGEN_H
#define BLOOM_MATERIALCOMPILER_CODEGEN_H

#include <filesystem>
#include <memory>
#include <string>

#include <utl/hashtable.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Core/Base.h"
#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

struct CodegenResult {
    std::string source;
    utl::hashmap<int, AssetHandle> fragmentTextures;
};

class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;

    virtual CodegenResult run() = 0;

protected:
    CodeGenerator() = default;
};

struct CodeGenOptions {};

BLOOM_API std::unique_ptr<CodeGenerator> makeCodeGenerator(
    Context const& ctx, MaterialGraph const& graph,
    CodeGenOptions const& options);

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_CODEGEN_H
