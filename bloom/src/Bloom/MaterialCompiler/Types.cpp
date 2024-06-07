#include "Bloom/MaterialCompiler/Types.h"

#include <sstream>

#include <magic_enum.hpp>
#include <utl/strcat.hpp>

using namespace bloom;
using namespace matc;

VectorType::VectorType(int numComponents):
    Type(TypeCategory::VectorType, utl::strcat("Float", numComponents)),
    numComps(numComponents) {}

AggregateType::AggregateType(std::string name,
                             std::vector<AggregateElement> elements):
    Type(TypeCategory::AggregateType, std::move(name)),
    _elements(std::move(elements)) {}

TextureType::TextureType(TextureSubType subtype):
    Type(TypeCategory::TextureType,
         std::string(magic_enum::enum_name(subtype))),
    _subtype(subtype) {}

static std::string functionTypeName(Type const* returnType,
                                    std::span<Type const* const> argTypes) {
    std::stringstream sstr;
    sstr << returnType->name() << "(";
    bool first = true;
    for (auto* type: argTypes) {
        if (!first) {
            sstr << ", ";
        }
        first = false;
        sstr << type->name();
    }
    sstr << ")";
    return std::move(sstr).str();
}

FunctionType::FunctionType(Type const* returnType,
                           std::vector<Type const*> argTypes):
    Type(TypeCategory::FunctionType, functionTypeName(returnType, argTypes)),
    ret(returnType),
    args(std::move(argTypes)) {}
