#include "Bloom/MaterialCompiler/Fwd.h"

using namespace bloom;
using namespace matc;

std::string_view matc::toSpelling(BuiltinFunctionKind function) {
    switch (function) {
#define BL_MATGRAPH_BUILTIN(Name, Spelling, ...)                               \
    case BuiltinFunctionKind::Name:                                            \
        return #Spelling;
#include "Bloom/MaterialCompiler/Lists.def"
    }
}
