#ifndef BLOOM_MATERIALCOMPILER_TYPES_H
#define BLOOM_MATERIALCOMPILER_TYPES_H

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

class VectorType: public Type {
public:
    explicit VectorType(int numComponents);

    int numComponents() const { return numComps; }

private:
    int numComps;
};

class AggregateType: public Type {
public:
    AggregateType(std::vector<Type const*> elements);

    std::span<Type const* const> elements() const { return _elements; }

private:
    std::vector<Type const*> _elements;
};

class TextureType: public Type {
public:
    TextureType(): Type(TypeCategory::TextureType, "Texture") {}
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_TYPES_H
