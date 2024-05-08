#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIAL_H

#include <memory>

#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/Fwd.h"

namespace bloom {

struct MaterialModel {
    virtual ~MaterialModel() = default;
    virtual YAML::Node serialize() const = 0;
    virtual void deserialize(YAML::Node const&) = 0;
    virtual void compile(Material&) const = 0;
};

__attribute__((weak)) std::unique_ptr<MaterialModel> makeMaterialModel();

class BLOOM_API Material: public Asset {
public:
    explicit Material(AssetHandle handle, std::string name);

    /// Reset this material to be the default material.
    /// We will probably change this in the future but for now the default
    /// material is the only material there really is
    void makeDefault(HardwareDevice& device);

    std::unique_ptr<MaterialModel> model;

    /// Main pass for the render graph. We will clean this up in the future
    RenderPipelineHandle rgMainPass;
    TriangleCullMode cullMode = TriangleCullMode::Back;
    DepthStencilHandle depthStencil;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
