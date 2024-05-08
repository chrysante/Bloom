#ifndef BLOOM_GRAPHICS_MATERIAL_CONTEXT_H
#define BLOOM_GRAPHICS_MATERIAL_CONTEXT_H

#include <span>
#include <vector>

#include <utl/hashtable.hpp>

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class Context {
public:
    Context();
    Context(Context&&) noexcept;
    Context& operator=(Context&&) noexcept;
    ~Context();

    FloatType const* floatType() const;
    Float2Type const* float2Type() const;
    Float3Type const* float3Type() const;
    Float4Type const* float4Type() const;

    AggregateType const* aggregateType(std::span<Type const* const> elements);

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace bloom::matc

#endif // BLOOM_GRAPHICS_MATERIAL_CONTEXT_H
