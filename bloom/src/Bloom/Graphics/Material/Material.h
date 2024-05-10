#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIAL_H

#include <memory>
#include <string>

#include <utl/hashtable.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

struct MaterialSourceData {
    std::string source;
    utl::hashmap<int, AssetHandle> fragmentTextures;
};

struct MaterialModel {
    virtual ~MaterialModel() = default;
    virtual bool hasCPURepr() const = 0;
    virtual YAML::Node serialize() const = 0;
    virtual void deserialize(YAML::Node const&) = 0;
    virtual MaterialSourceData generateSource() const = 0;
};

__attribute__((weak)) std::unique_ptr<MaterialModel> makeMaterialModel();

class BLOOM_API Material: public Asset {
public:
    explicit Material(AssetHandle handle, std::string name);

    void compile(HardwareDevice& device, AssetManager& AM);

    std::unique_ptr<MaterialModel> model;

    /// Main pass for the render graph. We will clean this up in the future
    RenderPipelineHandle rgMainPass;
    TriangleCullMode cullMode = TriangleCullMode::Back;
    DepthStencilHandle depthStencil;
    utl::hashmap<int, Reference<Texture2D>> fragmentTextures;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
