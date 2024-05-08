#include "Bloom/Graphics/Material/Types.h"

#include <sstream>

using namespace bloom;
using namespace matgraph;

std::string AggregateType::makeName(std::span<Type const*> elements) {
    std::stringstream sstr;
    sstr << "{ ";
    bool first = true;
    for (auto* type: elements) {
        if (!first) {
            sstr << ", ";
        }
        first = false;
        sstr << type->name();
    }
    sstr << " }";
    return std::move(sstr).str();
}
