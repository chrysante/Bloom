#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIAL_H

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/GPU/HardwarePrimitives.h"

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

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIAL_H
