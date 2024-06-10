#ifndef BLOOM_MATERIALCOMPILER_TYPES_H
#define BLOOM_MATERIALCOMPILER_TYPES_H

#include <span>
#include <string>
#include <vector>

#include <magic_enum.hpp>

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class Type {
public:
    std::string const& name() const { return _name; }

protected:
    explicit Type(TypeCategory rtti, std::string name):
        _rtti(rtti), _name(std::move(name)) {}

private:
    friend TypeCategory dyncast_get_type(Type const& type) {
        return type._rtti;
    }

    TypeCategory _rtti;
    std::string _name;
};

class VoidType: public Type {
public:
    VoidType(): Type(TypeCategory::VoidType, "Void") {}
};

class BoolType: public Type {
public:
    BoolType(): Type(TypeCategory::BoolType, "Bool") {}
};

class IntType: public Type {
public:
    explicit IntType(IntSubType subtype):
        Type(TypeCategory::IntType,
             std::string(magic_enum::enum_name(subtype))),
        _subtype(subtype) {}

    IntSubType subtype() const { return _subtype; }

private:
    IntSubType _subtype;
};

class VectorType: public Type {
public:
    explicit VectorType(int numComponents,
                        VectorTypeFlags flags = VectorTypeFlags::None);

    int numComponents() const { return numComps; }

    VectorTypeFlags flags() const { return _flags; }

    bool packed() const { return test(flags() & VectorTypeFlags::Packed); }

private:
    int numComps;
    VectorTypeFlags _flags;
};

class MatrixType: public Type {
public:
    explicit MatrixType(int rows, int columns);

    int rows() const { return _rows; }

    int columns() const { return _columns; }

private:
    int _rows;
    int _columns;
};

class AggregateType: public Type {
public:
    AggregateType(std::string name, std::vector<AggregateElement> elements,
                  bool isExtern = false);

    std::span<AggregateElement const> elements() const { return _elements; }

    AggregateElement element(size_t index) const { return _elements[index]; }

    bool isExtern() const { return _isExtern; }

private:
    std::vector<AggregateElement> _elements;
    bool _isExtern = false;
};

class TextureType: public Type {
public:
    explicit TextureType(TextureSubType subtype);

    TextureSubType subtype() const { return _subtype; }

private:
    TextureSubType _subtype;
};

class FunctionType: public Type {
public:
    explicit FunctionType(Type const* returnType,
                          std::vector<Type const*> argTypes);

    Type const* returnType() const { return ret; }

    std::span<Type const* const> argumentTypes() const { return args; }

private:
    Type const* ret;
    std::vector<Type const*> args;
};

class PointerType: public Type {
public:
    explicit PointerType(Type const* baseType, Mutability mutability);

    Type const* baseType() const { return base; }

    Mutability mutability() const { return mut; }

    bool isConst() const { return mutability() == Mutability::Const; }

    bool isMut() const { return mutability() == Mutability::Mutable; }

private:
    Type const* base;
    Mutability mut;
};

class ArrayType: public Type {
public:
    explicit ArrayType(Type const* elementType, IntConstant const* count);

    Type const* elementType() const { return elemType; }

    IntConstant const* count() const { return _count; }

private:
    Type const* elemType;
    IntConstant const* _count;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_TYPES_H
