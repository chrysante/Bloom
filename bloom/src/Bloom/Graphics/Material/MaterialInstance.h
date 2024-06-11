#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALINSTANCE_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALINSTANCE_H

#include <span>

#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/Serialization.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialSourceData.h"

namespace bloom {

class AssetManager;

class BLOOM_API MaterialInstance:
    public Asset,
    public EnableRefFromThis<MaterialInstance> {
public:
    BL_DEFINE_ASSET_CTOR(MaterialInstance, Asset)

    Material* material() const { return mMaterial.get(); }

    Reference<Material> materialRef() const { return mMaterial; }

    void setMaterial(Reference<Material> material);

    BufferView buffer() const { return gpuBuffer; }

    std::span<MaterialPrimitiveParameter> getPrimitiveParameters() {
        return primitiveParameters;
    }

    std::span<MaterialTextureParameter> getTextureParameters() {
        return textureParameters;
    }

    void setParameter(void const* data, size_t offset, size_t size);

    void setTexture(size_t index, Reference<Texture2D> texture);

    Reference<Texture2D> getTexture(size_t index) const;

    YAML::Node serialize() const;

    void deserialize(YAML::Node const&, AssetManager&);

    /// Uploads parameters to the GPU if they have changed since the last call
    /// to this function
    void uploadData(HardwareDevice& device);

private:
    friend class ForwardRenderer;
    friend class AssetManager;

    Reference<Material> mMaterial;
    BufferHandle gpuBuffer;
    std::vector<uint8_t> cpuBuffer;
    std::vector<MaterialPrimitiveParameter> primitiveParameters;
    std::vector<MaterialTextureParameter> textureParameters;
    std::vector<Reference<Texture2D>> textures;
    bool mDirty = true;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALINSTANCE_H
