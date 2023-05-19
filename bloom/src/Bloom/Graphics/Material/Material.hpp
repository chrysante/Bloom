#pragma once

#include "Bloom/Asset/Asset.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"

namespace bloom {

class HardwareDevice;

class BLOOM_API Material: public Asset {
public:
    static Material makeDefaultMaterial(HardwareDevice&, Asset base);

    using Asset::Asset;
    Material(Asset asset): Asset(std::move(asset)) {}

    RenderPipelineHandle mainPass;
    RenderPipelineHandle editorPass;
    RenderPipelineHandle outlinePass;
    TriangleCullMode cullMode = TriangleCullMode::back;
    void* functionTable;
};

} // namespace bloom
