#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace bloom;
using namespace matc;

Value* MaterialGraph::addValue(DynUniquePtr<Value> value) {
    _values.push_back(std::move(value));
    return _values.back().get();
}
