#include "Bloom/Graphics/Material/Material.h"

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Material/MaterialGraph.h"

using namespace bloom;

Material::Material(AssetHandle handle, std::string name):
    Asset(handle, std::move(name)) {
    BL_EXPECT(handle.type() == AssetType::Material, "Mismatched asset type");
}

Material::~Material() = default;

void Material::makeDefault(HardwareDevice& device) {
    rgMainPass = device.createRenderPipeline(
        { .colorAttachments = { { .pixelFormat = PixelFormat::RGBA32Float },
                                { .pixelFormat = PixelFormat::R32Uint } },
          .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = device.createFunction("rgMainPassVS"),
          .fragmentFunction = device.createFunction("rgMainPassFS") });
    depthStencil = device.createDepthStencil(
        { .depthWrite = true,
          .depthCompareFunction = CompareFunction::LessEqual });
}
