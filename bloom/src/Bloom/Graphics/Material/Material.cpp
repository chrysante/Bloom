#include "Bloom/Graphics/Material/Material.h"

#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;

void Material::makeDefault(HardwareDevice& device) {
    RenderPipelineDescription desc;
    ColorAttachmentDescription ca;
    // Main Pass
    ca.pixelFormat = PixelFormat::RGBA32Float;
    desc.colorAttachments.push_back(ca);
    desc.vertexFunction = device.createFunction("mainPassVS");
    desc.fragmentFunction = device.createFunction("mainPassFS");
    desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
    mainPass = device.createRenderPipeline(desc);
#if 0 // TODO: Reenable this
    // Editor Pass
    ca.pixelFormat = PixelFormat::RGBA32Float;
    desc.colorAttachments.push_back(ca);
    desc.vertexFunction   = device.createFunction("editorPassVS");
    desc.fragmentFunction = device.createFunction("editorPassFS");
    desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
    editorPass = device.createRenderPipeline(desc);
#endif
    // Outline Pass
    desc.colorAttachments[0].pixelFormat = PixelFormat::R8Unorm;
    desc.vertexFunction = device.createFunction("outlinePassVS");
    desc.fragmentFunction = device.createFunction("outlinePassFS");
    outlinePass = device.createRenderPipeline(desc);
}
