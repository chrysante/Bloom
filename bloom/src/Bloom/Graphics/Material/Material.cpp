#include "Bloom/Graphics/Material/Material.h"

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/ShaderCompiler.h"
#include "Bloom/Graphics/Texture.h"

using namespace bloom;

std::unique_ptr<MaterialModel> bloom::makeMaterialModel() { BL_UNREACHABLE(); }

Material::Material(AssetHandle handle, std::string name):
    Asset(handle, std::move(name)) {
    BL_EXPECT(handle.type() == AssetType::Material, "Mismatched asset type");
    model = makeMaterialModel();
}

static RenderPipelineHandle createPipeline(
    HardwareDevice& device, ShaderFunctionHandle vertexFunction,
    ShaderFunctionHandle fragmentFunction) {
    return device.createRenderPipeline(
        { .colorAttachments = { { .pixelFormat = PixelFormat::RGBA32Float },
                                { .pixelFormat = PixelFormat::R32Uint } },
          .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = vertexFunction,
          .fragmentFunction = fragmentFunction });
}

void Material::compile(HardwareDevice& device, AssetManager& AM) {
    try {
        auto sourceData = model->generateSource();
        std::vector<std::string> functionNames = { "materialVertexShader",
                                                   "materialFragmentShader" };
        auto shaderCompiler = device.createShaderCompiler();
        auto functions =
            shaderCompiler->compile(sourceData.source, functionNames);
        BL_ASSERT(functions.size() == 2);
        rgMainPass = createPipeline(device, functions[0], functions[1]);
        for (auto [index, assetHandle]: sourceData.fragmentTextures) {
            fragmentTextures[index] =
                as<Texture2D>(AM.get(assetHandle, AssetRepresentation::GPU));
        }
    }
    catch (std::exception const& e) {
        Logger::Error("Failed to compile material: ", e.what());
        Logger::Trace("Resorting to default material");
        rgMainPass = createPipeline(device,
                                    device.createFunction("rgMainPassVS"),
                                    device.createFunction("rgMainPassFS"));
    }
    depthStencil = device.createDepthStencil(
        { .depthWrite = true,
          .depthCompareFunction = CompareFunction::LessEqual });
}
