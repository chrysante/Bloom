#ifndef BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
#define BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H

#include <concepts>
#include <functional>
#include <memory>
#include <span>
#include <variant>
#include <vector>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Utils.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

namespace bloom::matc {

class MaterialGraph {
public:
    Value* addValue(DynUniquePtr<Value> value);

    template <std::derived_from<Value> T>
    T* addValue(DynUniquePtr<T> value) {
        auto* result = addValue(DynUniquePtr<Value>(value.release()));
        return cast<T*>(result);
    }

    template <std::derived_from<Value> T, typename... Args>
        requires std::constructible_from<T, Args...>
    T* addValue(Args&&... args) {
        return addValue(allocate<T>(std::forward<Args>(args)...));
    }

    Value* addVectorParameter(Context& ctx, std::string name, int numComponents,
                              vml::float4 defaultValue);

    TextureConstant* addTextureParameter(Context& ctx, std::string name,
                                         TextureSubType subtype,
                                         AssetHandle asset);

    void setRoot(GraphRoot root, Value* value) { _roots[root] = value; }

    auto values() const {
        return _values | ranges::views::transform(
                             [](auto& p) -> auto const* { return p.get(); });
    }

    ///
    utl::hashmap<GraphRoot, Value*> const& roots() const { return _roots; }

    utl::hashset<Value*> const& userParameters() const { return _userParams; }

private:
    std::vector<DynUniquePtr<Value>> _values;
    utl::hashmap<GraphRoot, Value*> _roots{};
    utl::hashset<Value*> _userParams;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
