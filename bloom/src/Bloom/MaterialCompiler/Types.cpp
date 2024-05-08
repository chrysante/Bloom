#include "Bloom/MaterialCompiler//Types.h"

#include <sstream>

using namespace bloom;
using namespace matc;

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
