#include "Bloom/MaterialCompiler/Function.h"

#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;

Type const* Function::returnType() const {
    if (auto* r = root()) {
        return r->type();
    }
    return nullptr;
}

Value* Function::root() const {
    if (_body.empty()) {
        return nullptr;
    }
    return _body.back();
}
