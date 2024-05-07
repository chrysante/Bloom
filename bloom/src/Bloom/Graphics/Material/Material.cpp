#include "Bloom/Graphics/Material/Material.h"

#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;

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
