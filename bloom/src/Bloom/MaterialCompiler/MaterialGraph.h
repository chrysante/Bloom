#ifndef BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
#define BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H

#include <concepts>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/EnumCount.h"
#include "Bloom/Core/Utils.h"
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
    explicit MaterialGraph(CodegenParameters codegenParams):
        codegenParams(codegenParams) {}

    void setRoot(GraphRoot root, Value* value) { _roots[root] = value; }

    Value* getRoot(GraphRoot root) const { return _roots[root]; }

    std::span<Value* const> roots() const { return _roots; }

    Function* getFunction(GraphRoot root) const {
        return _functions[root].get();
    }

    auto functions() const {
        return _functions | ranges::views::transform(ToAddress<>);
    }

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

    auto values() const {
        return _values | ranges::views::transform(
                             [](auto& p) -> auto const* { return p.get(); });
    }

    std::span<VectorConstant* const> vectorParameters() const {
        return _vecParams;
    }

    std::span<TextureConstant* const> textureParameters() const {
        return _texParams;
    }

    void buildFunctions(Context& ctx);

    void generateShaderFunctions(Context& ctx);

    CodegenParameters const& codegenParameters() const { return codegenParams; }

private:
    std::vector<DynUniquePtr<Value>> _values;
    EnumArray<GraphRoot, Value*> _roots{};
    EnumArray<GraphRoot, DynUniquePtr<Function>> _functions{};
    std::vector<VectorConstant*> _vecParams;
    std::vector<TextureConstant*> _texParams;
    CodegenParameters codegenParams;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
