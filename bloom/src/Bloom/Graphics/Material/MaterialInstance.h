#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALINSTANCE_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALINSTANCE_H

#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/Serialization.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialParameters.h"

namespace bloom {

class AssetManager;

class BLOOM_API MaterialInstance: public Asset {
    friend class ForwardRenderer;
    friend class AssetManager;

public:
    BL_DEFINE_ASSET_CTOR(MaterialInstance, Asset)

    Material* material() { return mMaterial.get(); }
    void setMaterial(Reference<Material>);
    BufferView parameterBuffer() const { return mParameterBuffer; }

    MaterialParameters const& paramaters() const { return mParameters; }
    void setParamaters(MaterialParameters const& params);

    YAML::Node serialize() const;
    void deserialize(YAML::Node const&, AssetManager&);

private:
    Reference<Material> mMaterial;
    BufferHandle mParameterBuffer;
    MaterialParameters mParameters;
    bool mDirty = true;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALINSTANCE_H
