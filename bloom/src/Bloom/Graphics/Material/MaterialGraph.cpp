#include "Bloom/Graphics/Material/MaterialGraph.h"

using namespace bloom;
using namespace matgraph;

Value* MaterialGraph::addValue(DynUniquePtr<Value> value) {
    _values.push_back(std::move(value));
    return _values.back().get();
}

std::optional<std::string> MaterialGraph::generateCode(
    MaterialCodeGenOptions const& options) {
    return std::nullopt;
}
