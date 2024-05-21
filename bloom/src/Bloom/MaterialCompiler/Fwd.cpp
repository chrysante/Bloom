#include "Bloom/MaterialCompiler/Fwd.h"

using namespace bloom;
using namespace matc;

std::string_view matc::toSpelling(BuiltinFunction function) {
    switch (function) {
#define BL_MATGRAPH_BUILTIN(Name, Spelling, ...)                               \
    case BuiltinFunction::Name:                                                \
        return #Spelling;
#include "Bloom/MaterialCompiler/Lists.def"
    }
}
