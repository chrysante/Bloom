#ifndef BLOOM_GRAPHICS_MATERIAL_TYPES_H
#define BLOOM_GRAPHICS_MATERIAL_TYPES_H

#include <span>
#include <string>
#include <vector>

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

class FloatType: public Type {
public:
    FloatType(): Type(TypeCategory::FloatType, "Float") {}
};

class Float2Type: public Type {
public:
    Float2Type(): Type(TypeCategory::Float2Type, "Float2") {}
};

class Float3Type: public Type {
public:
    Float3Type(): Type(TypeCategory::Float3Type, "Float3") {}
};

class Float4Type: public Type {
public:
    Float4Type(): Type(TypeCategory::Float4Type, "Float4") {}
};

class AggregateType: public Type {
public:
    static std::string makeName(std::span<Type const*> elements);

    AggregateType(std::vector<Type const*> elements):
        Type(TypeCategory::AggregateType, makeName(elements)),
        _elements(std::move(elements)) {}

    std::span<Type const* const> elements() const { return _elements; }

private:
    std::vector<Type const*> _elements;
};

} // namespace bloom::matc

#endif // BLOOM_GRAPHICS_MATERIAL_TYPES_H
