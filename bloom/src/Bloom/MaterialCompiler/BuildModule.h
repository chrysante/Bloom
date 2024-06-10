#ifndef BLOOM_MATERIALCOMPILER_BUILDMODULE_H
#define BLOOM_MATERIALCOMPILER_BUILDMODULE_H

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

Module buildModule(Context& ctx, MaterialGraph const& G,
                   struct CodegenParameters codegenParams);

}

#endif // BLOOM_MATERIALCOMPILER_BUILDMODULE_H
