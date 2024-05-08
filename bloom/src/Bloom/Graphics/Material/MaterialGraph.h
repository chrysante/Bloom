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
#include "Bloom/Graphics/Material/Context.h"
#include "Bloom/Graphics/Material/Types.h"
#include "Bloom/Graphics/Material/Values.h"

namespace bloom::matgraph {

struct MaterialCodeGenOptions {
    std::string declarations;
};

class MaterialGraph {
public:
    Context& context() { return ctx; }

    template <std::derived_from<Value> T>
    T* setRoot(DynUniquePtr<T> value) {
        BL_EXPECT(!_root);
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

    std::optional<std::string> generateCode(
        MaterialCodeGenOptions const& options);

private:
    Context ctx;
    std::vector<DynUniquePtr<Value>> _values;
    Value* _root = nullptr;
};

} // namespace bloom::matgraph

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALGRAPH_H
