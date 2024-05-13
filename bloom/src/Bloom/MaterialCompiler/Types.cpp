#include "Bloom/MaterialCompiler//Types.h"

#include <sstream>

#include <magic_enum.hpp>
#include <utl/strcat.hpp>

using namespace bloom;
using namespace matc;

VectorType::VectorType(int numComponents):
    Type(TypeCategory::VectorType, utl::strcat("Float", numComponents)),
    numComps(numComponents) {}

static std::string makeAggregateName(std::span<Type const*> elements) {
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

AggregateType::AggregateType(std::vector<Type const*> elements):
    Type(TypeCategory::AggregateType, makeAggregateName(elements)),
    _elements(std::move(elements)) {}

TextureType::TextureType(TextureSubType subtype):
    Type(TypeCategory::TextureType,
         std::string(magic_enum::enum_name(subtype))),
    _subtype(subtype) {}
