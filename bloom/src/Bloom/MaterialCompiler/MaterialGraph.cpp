#include "Bloom/MaterialCompiler/MaterialGraph.h"

#include <range/v3/view.hpp>

using namespace bloom;
using namespace matc;
using namespace ranges::views;

Value* MaterialGraph::addValue(DynUniquePtr<Value> value) {
    _values.push_back(std::move(value));
    return _values.back().get();
}
