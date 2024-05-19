#ifndef BLOOM_MATERIALCOMPILER_CODEGEN_H
#define BLOOM_MATERIALCOMPILER_CODEGEN_H

#include <memory>
#include <string>
#include <vector>

#include <utl/hashtable.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;

    virtual MaterialSourceData run() = 0;

protected:
    CodeGenerator() = default;
};

struct CodeGenOptions {};

BLOOM_API std::unique_ptr<CodeGenerator> makeCodeGenerator(
    Context& ctx, MaterialGraph const& graph, CodeGenOptions const& options);

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_CODEGEN_H
