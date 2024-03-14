#pragma once

#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.hpp"
#include "Bloom/Asset/Serialization.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"
#include "Bloom/Graphics/Material/Material.hpp"
#include "Bloom/Graphics/Material/MaterialParameters.hpp"

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
