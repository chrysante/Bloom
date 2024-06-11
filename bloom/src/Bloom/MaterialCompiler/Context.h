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

    VoidType const* voidType();
    BoolType const* boolType();
    IntType const* intType(IntSubType subtype);
    VectorType const* vectorType(int numComponents,
                                 VectorTypeFlags flags = VectorTypeFlags::None);
    VectorType const* floatType(VectorTypeFlags flags = VectorTypeFlags::None) {
        return vectorType(1, flags);
    }
    VectorType const* float2Type(
        VectorTypeFlags flags = VectorTypeFlags::None) {
        return vectorType(2, flags);
    }
    VectorType const* float3Type(
        VectorTypeFlags flags = VectorTypeFlags::None) {
        return vectorType(3, flags);
    }
    VectorType const* float4Type(
        VectorTypeFlags flags = VectorTypeFlags::None) {
        return vectorType(4, flags);
    }
    MatrixType const* matrixType(int rows, int columns);
    AggregateType const* aggregateType(
        std::string name, std::span<AggregateElement const> elements,
        bool isExtern = false);
    AggregateType const* aggregateType(
        std::string name, std::initializer_list<AggregateElement> elements,
        bool isExtern = false) {
        return aggregateType(std::move(name), std::span(elements), isExtern);
    }
    TextureType const* textureType(TextureSubType subtype);
    FunctionType const* functionType(Type const* returnType,
                                     std::span<Type const* const> argTypes);
    PointerType const* pointerType(Type const* baseType, Mutability mutabiliy);
    ArrayType const* arrayType(Type const* elementType,
                               IntConstant const* count);
    ArrayType const* arrayType(Type const* elementType, int count);

    IntConstant* intConstant(IntSubType subtype, long long value);
    VectorConstant* vectorConstant(int numComponents, vml::float4 value);
    VectorConstant* floatConstant(float value);
    VectorConstant* float2Constant(vml::float2 value);
    VectorConstant* float3Constant(vml::float3 value);
    VectorConstant* float4Constant(vml::float4 value);

    TextureConstant* textureConstant(TextureSubType subtype, AssetHandle asset);

    RasterizerInput* rasterizerInput(RasterizerInputKind kind);

    SceneDataInput* sceneData(SceneDataKind kind);

    BuiltinFunction* builtinFunction(BuiltinFunctionKind kind,
                                     std::span<Type const* const> args);

    ///  \Returns a list of all existing types
    std::vector<Type const*> types() const;

    ///  \Returns a list of all existing texture constants
    std::vector<TextureConstant*> textureConstants() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_CONTEXT_H
