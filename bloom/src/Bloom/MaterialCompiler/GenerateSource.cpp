#include "Bloom/MaterialCompiler/Codegen.h"

#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/MaterialCompiler/Module.h"

using namespace bloom;
using namespace matc;

MaterialSourceData matc::generateSource(Context& ctx, MaterialGraph const& G,
                                        CodegenParameters codegenParams) {
    MaterialSourceData sourceData;
    auto mod = buildModule(ctx, G, sourceData, codegenParams);
    auto generator = makeCodeGenerator(ctx, mod);
    sourceData.source = generator->run();
    return sourceData;
}
