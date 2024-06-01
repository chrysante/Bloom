#ifndef BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
#define BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H

#include <concepts>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <utl/hashtable.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/EnumCount.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

namespace bloom::matc {

struct CodegenParameters {
    MaterialShadingModel shadingModel = {};
    MaterialTransparency transparency = {};
    bool twoSided = false;
};

class MaterialGraph {
public:
    explicit MaterialGraph(CodegenParameters params): params(params) {}

    void setRoot(GraphRoot root, Value* value) { _roots[root] = value; }

    Value* getRoot(GraphRoot root) const { return _roots[root]; }

    std::span<Value* const> roots() const { return _roots; }

    Value* addValue(DynUniquePtr<Value> value);

    template <std::derived_from<Value> T>
    T* addValue(DynUniquePtr<T> value) {
        auto* result = addValue(DynUniquePtr<Value>(value.release()));
        return cast<T*>(result);
    }

    template <std::derived_from<Value> T, typename... Args>
    T* addValue(Args&&... args) {
        return addValue(allocate<T>(std::forward<Args>(args)...));
    }

    auto values() const {
        return _values | ranges::views::transform(
                             [](auto& p) -> auto const* { return p.get(); });
    }

    CodegenParameters const& parameters() const { return params; }

private:
    std::vector<DynUniquePtr<Value>> _values;
    EnumArray<GraphRoot, Value*> _roots{};
    CodegenParameters params;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
