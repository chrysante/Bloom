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
#include "Bloom/Graphics/Material/MaterialSourceData.h"
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

    /// \Returns a view over all "functions" and "shaders" defined by the
    /// material
    utl::small_vector<Function*> functions() const;

    ///
    Function* getFunction(GraphRoot kind) const;

    ///
    utl::small_vector<Function*> getFunctions(
        std::span<GraphRoot const> kinds) const;

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

    MaterialSourceData generate(Context& ctx);

    auto values() const {
        return _values | ranges::views::transform(
                             [](auto& p) -> auto const* { return p.get(); });
    }

    CodegenParameters const& codegenParameters() const { return codegenParams; }

private:
    void buildFunctions(Context& ctx, MaterialSourceData& sourceData);
    DynUniquePtr<Function> buildFunction(Context& ctx, Value* root,
                                         std::string name,
                                         utl::hashset<Value*>& usedParams);
    void computeParameters(
        Context& ctx, Function& F, AggregateType const* paramBufferType,
        utl::hashset<Value*> const& usedParams,
        std::span<VectorConstant const* const> vectorParameters);

    std::vector<DynUniquePtr<Value>> _values;
    utl::hashmap<GraphRoot, Value*> _roots{};
    /// Functions defined by the material editor and shared between shaders
    utl::hashmap<GraphRoot, DynUniquePtr<Function>> _functions{};
    /// Different shaders exposed to the renderer
    utl::hashmap<MaterialShaderType, DynUniquePtr<Function>> _shaders{};
    utl::hashset<Value*> _params;
    CodegenParameters codegenParams;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_MATERIALGRAPH_H
