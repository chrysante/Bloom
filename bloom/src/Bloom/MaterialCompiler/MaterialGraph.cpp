#include "Bloom/MaterialCompiler/MaterialGraph.h"

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hashmap.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Context.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

Value* MaterialGraph::addValue(DynUniquePtr<Value> value) {
    _values.push_back(std::move(value));
    return _values.back().get();
}

Value* MaterialGraph::addVectorParameter(Context& ctx, std::string name,
                                         int numComponents,
                                         vml::float4 defaultValue) {
    auto value = allocate<VectorConstant>(ctx, std::move(name), numComponents,
                                          defaultValue);
    _userParams.insert(value.get());
    return addValue(std::move(value));
}

TextureConstant* MaterialGraph::addTextureParameter(Context& ctx,
                                                    std::string name,
                                                    TextureSubType subtype,
                                                    AssetHandle asset) {
    auto value = allocate<TextureConstant>(std::move(name),
                                           ctx.textureType(subtype), asset);
    _userParams.insert(value.get());
    return addValue(std::move(value));
}

Value* MaterialGraph::getRoot(GraphRoot kind) const {
    auto itr = _roots.find(kind);
    return itr != _roots.end() ? itr->second : nullptr;
}
