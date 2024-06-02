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
    AggregateType(std::string name, std::vector<AggregateElement> elements);

    std::span<AggregateElement const> elements() const { return _elements; }

    AggregateElement element(size_t index) const { return _elements[index]; }

private:
    std::vector<AggregateElement> _elements;
};

class TextureType: public Type {
public:
    explicit TextureType(TextureSubType subtype);

    TextureSubType subtype() const { return _subtype; }

private:
    TextureSubType _subtype;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_TYPES_H
