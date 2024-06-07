#ifndef BLOOM_MATERIALCOMPILER_CONTEXT_H
#define BLOOM_MATERIALCOMPILER_CONTEXT_H

#include <span>
#include <vector>

#include <utl/hashtable.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class Context {
public:
    Context();
    Context(Context&&) noexcept;
    Context& operator=(Context&&) noexcept;
    ~Context();

    VectorType const* vectorType(int numComponents) const;
    VectorType const* floatType() const { return vectorType(1); }
    VectorType const* float2Type() const { return vectorType(2); }
    VectorType const* float3Type() const { return vectorType(3); }
    VectorType const* float4Type() const { return vectorType(4); }

    VectorConstant* vectorConstant(int numComponents, vml::float4 value);
    VectorConstant* floatConstant(float value);
    VectorConstant* float2Constant(vml::float2 value);
    VectorConstant* float3Constant(vml::float3 value);
    VectorConstant* float4Constant(vml::float4 value);

    TextureConstant* textureConstant(TextureSubType subtype, AssetHandle asset);

    RasterizerInput* rasterizerInput(RasterizerInputKind kind);

    BuiltinFunction* builtinFunction(BuiltinFunctionKind kind,
                                     std::span<Type const* const> args);

    AggregateType const* aggregateType(
        std::string name, std::span<AggregateElement const> elements);
    AggregateType const* aggregateType(
        std::string name, std::initializer_list<AggregateElement> elements) {
        return aggregateType(std::move(name), std::span(elements));
    }

    TextureType const* textureType(TextureSubType subtype);

    FunctionType const* functionType(Type const* returnType,
                                     std::span<Type const* const> argTypes);

    ///  \Returns a list of all existing types
    std::vector<Type const*> types() const;

    ///  \Returns a list of all existing texture constants
    std::vector<TextureConstant const*> textureConstants() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_CONTEXT_H
