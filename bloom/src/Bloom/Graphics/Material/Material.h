#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIAL_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <utl/hashtable.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/MaterialSourceData.h"

namespace bloom {

struct MaterialSourceData;

struct MaterialModel {
    virtual ~MaterialModel() = default;
    virtual bool hasCPURepr() const = 0;
    virtual YAML::Node serialize() const = 0;
    virtual void deserialize(YAML::Node const&) = 0;
    virtual MaterialSourceData generateSource() const = 0;
};

__attribute__((weak)) std::unique_ptr<MaterialModel> makeMaterialModel();

class BLOOM_API Material: public Asset, public EnableRefFromThis<Material> {
public:
    explicit Material(AssetHandle handle, std::string name);

    void compile(HardwareDevice& device, AssetManager& AM);

    std::unique_ptr<MaterialModel> model;

    Reference<ShaderLibrary> shaderLibrary;
    RenderPipelineHandle rgMainPass;
    TriangleCullMode cullMode = TriangleCullMode::Back;
    DepthStencilHandle depthStencil;
    utl::hashmap<int, Reference<Texture2D>> constantFragmentTextures;
    std::vector<MaterialPrimitiveParameter> primitiveParameters;
    size_t paramBufferSize = 0;
    std::vector<MaterialTextureParameter> textureParameters;
    std::set<WeakReference<MaterialInstance>, std::owner_less<>> instances;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
