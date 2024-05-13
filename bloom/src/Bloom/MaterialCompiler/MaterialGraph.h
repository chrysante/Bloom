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

#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

namespace bloom::matc {

class MaterialGraph {
public:
    explicit MaterialGraph(MaterialShadingModel shadingModel):
        _shadingModel(shadingModel) {}

    void setRoot(Value* value) { _root = value; }

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

    Value* root() const { return _root; }

    auto values() const {
        return _values | ranges::views::transform(
                             [](auto& p) -> auto const* { return p.get(); });
    }

    MaterialShadingModel shadingModel() const { return _shadingModel; }

private:
    std::vector<DynUniquePtr<Value>> _values;
    Value* _root = nullptr;
    MaterialShadingModel _shadingModel;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
