#include "Bloom/Graphics/Material/Material.h"

#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;

std::unique_ptr<MaterialModel> bloom::makeMaterialModel() { BL_UNREACHABLE(); }

Material::Material(AssetHandle handle, std::string name):
    Asset(handle, std::move(name)) {
    BL_EXPECT(handle.type() == AssetType::Material, "Mismatched asset type");
    model = makeMaterialModel();
}

void Material::compile(HardwareDevice& device,
                       ShaderFunctionHandle vertexFunction,
                       ShaderFunctionHandle fragmentFunction) {
    rgMainPass = device.createRenderPipeline(
        { .colorAttachments = { { .pixelFormat = PixelFormat::RGBA32Float },
                                { .pixelFormat = PixelFormat::R32Uint } },
          .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = vertexFunction,
          .fragmentFunction = fragmentFunction });
    depthStencil = device.createDepthStencil(
        { .depthWrite = true,
          .depthCompareFunction = CompareFunction::LessEqual });
}

void Material::makeDefault(HardwareDevice& device) {
    compile(device, device.createFunction("rgMainPassVS"),
            device.createFunction("rgMainPassFS"));
}
