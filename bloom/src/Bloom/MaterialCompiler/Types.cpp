#include "Bloom/MaterialCompiler/Types.h"

#include <sstream>

#include <magic_enum.hpp>
#include <utl/strcat.hpp>

#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;

VectorType::VectorType(int numComponents, VectorTypeFlags flags):
    Type(TypeCategory::VectorType, utl::strcat("Float", numComponents)),
    numComps(numComponents),
    _flags(flags) {}

MatrixType::MatrixType(int rows, int columns):
    Type(TypeCategory::MatrixType, utl::strcat("Float", rows, "x", columns)),
    _rows(rows),
    _columns(columns) {}

AggregateType::AggregateType(std::string name,
                             std::vector<AggregateElement> elements,
                             bool isExtern):
    Type(TypeCategory::AggregateType, std::move(name)),
    _elements(std::move(elements)),
    _isExtern(isExtern) {}

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

static std::string makePointerName(Type const* baseType,
                                   Mutability mutability) {
    return utl::strcat(baseType->name(),
                       mutability == Mutability::Const ? " Const " : " Mut ",
                       "Pointer");
}

PointerType::PointerType(Type const* baseType, Mutability mutability):
    Type(TypeCategory::PointerType, makePointerName(baseType, mutability)),
    base(baseType),
    mut(mutability) {}

static std::string makeArrayName(Type const* elementType,
                                 IntConstant const* count) {
    return utl::strcat(elementType->name(), "[", count->get(), "]]");
}

ArrayType::ArrayType(Type const* elementType, IntConstant const* count):
    Type(TypeCategory::ArrayType, makeArrayName(elementType, count)),
    elemType(elementType),
    _count(count) {}
