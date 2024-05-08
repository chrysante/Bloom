#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALGRAPH_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALGRAPH_H

#include <concepts>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <utl/hashtable.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

namespace bloom::matc {

class MaterialGraph {
public:
    template <std::derived_from<Value> T>
    T* setRoot(DynUniquePtr<T> value) {
        assert(!_root);
        auto* result = addValue(std::move(value));
        _root = result;
        return result;
    }

    Value* addValue(DynUniquePtr<Value> value);

    template <std::derived_from<Value> T>
    T* addValue(DynUniquePtr<T> value) {
        auto* result = addValue(DynUniquePtr<Value>(value.release()));
        return cast<T*>(result);
    }

    Value* root() const { return _root; }

    auto values() const {
        return _values | ranges::views::transform(
                             [](auto& p) -> auto const* { return p.get(); });
    }

private:
    std::vector<DynUniquePtr<Value>> _values;
    Value* _root = nullptr;
};

} // namespace bloom::matc

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALGRAPH_H
