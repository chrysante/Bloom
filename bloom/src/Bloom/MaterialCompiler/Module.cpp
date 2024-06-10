#include "Bloom/MaterialCompiler/Module.h"

using namespace bloom;
using namespace matc;

Function* Module::addFunction(DynUniquePtr<Function> F) {
    auto* raw = F.get();
    _functions.push_back(std::move(F));
    return raw;
}
