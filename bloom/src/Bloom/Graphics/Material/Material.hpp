#pragma once

#include "Bloom/Asset/Asset.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"

namespace bloom {

class HardwareDevice;

class BLOOM_API Material: public Asset {
public:
    BL_DEFINE_ASSET_CTOR(Material, Asset)

    /// Reset this material to be the default material.
    /// We will probably change this in the future but for now the default
    /// material is the only material there really is
    void makeDefault(HardwareDevice& device);

    RenderPipelineHandle mainPass;
    RenderPipelineHandle editorPass;
    RenderPipelineHandle outlinePass;
    TriangleCullMode cullMode = TriangleCullMode::back;
    void* functionTable;
};

} // namespace bloom
