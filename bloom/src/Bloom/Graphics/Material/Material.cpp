#include "Bloom/Graphics/Material/Material.h"

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/ShaderCompiler.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/Graphics/Texture.h"

using namespace bloom;

std::unique_ptr<MaterialModel> bloom::makeMaterialModel() { BL_UNREACHABLE(); }

Material::Material(AssetHandle handle, std::string name):
    Asset(handle, std::move(name)) {
    BL_EXPECT(handle.type() == AssetType::Material, "Mismatched asset type");
    model = makeMaterialModel();
}

static RenderPipelineHandle createShadingPipeline(
    HardwareDevice& device, ShaderFunctionHandle vertexFunction,
    ShaderFunctionHandle fragmentFunction) {
    // clang-format off
    return device.createRenderPipeline({
        .colorAttachments = {
            { .pixelFormat = PixelFormat::RGBA32Float },
            { .pixelFormat = PixelFormat::R32Uint }
        },
        .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
        .vertexFunction = vertexFunction,
        .fragmentFunction = fragmentFunction
    }); // clang-format on
}

static RenderPipelineHandle createShadowPipeline(
    HardwareDevice& device, ShaderFunctionHandle vertexFunction,
    ShaderFunctionHandle fragmentFunction) {
    // clang-format off
    return device.createRenderPipeline({
        .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
        .inputPrimitiveTopology = PrimitiveTopologyClass::Triangle,
        .vertexFunction = vertexFunction,
        .fragmentFunction = fragmentFunction
    }); // clang-format on
}

static void updateInstances(
    Material& mat,
    std::set<WeakReference<MaterialInstance>, std::owner_less<>>& instances) {
    for (auto itr = instances.begin(); itr != instances.end();) {
        auto instance = itr->lock();
        if (!instance) {
            itr = instances.erase(itr);
            continue;
        }
        ++itr;
        instance->setMaterial(mat.shared_from_this());
    }
}

void Material::compile(HardwareDevice& device, AssetManager& AM) {
    BL_PROFILE_FUNCTION();
    Logger::Info("Compiling material: ", name());
    try {
        auto sourceData = model->generateSource();
        auto shaderCompiler = device.createShaderCompiler();
        auto library = shaderCompiler->compileSource(sourceData.source);
        if (!library) {
            throw std::runtime_error("Failed to compile shaders");
        }
        shaderLibrary = library;
        using enum MaterialShaderType;
        auto& names = sourceData.functionNames;
        auto shadingVS = library->createFunction(names[ShadingVertex]);
        auto shadingFS = library->createFunction(names[ShadingFragment]);
        rgMainPass = createShadingPipeline(device, shadingVS, shadingFS);
        if (!sourceData.functionNames[ShadowVertex].empty()) {
            auto shadowVS = library->createFunction(names[ShadowVertex]);
            auto shadowFS = library->createFunction(names[ShadowFragment]);
            shadowPipeline = createShadowPipeline(device, shadowVS, shadowFS);
        }
        else {
            shadowPipeline = {};
        }
        constantTextureParameters.clear();
        constantTextures.clear();
        for (auto& param: sourceData.textureConstants) {
            constantTextureParameters.push_back(param);
            auto tex =
                as<Texture2D>(AM.get(param.value, AssetRepresentation::GPU));
            constantTextures.push_back(tex);
        }
        primitiveParameters = std::move(sourceData.primitiveParameters);
        paramBufferSize = sourceData.parameterBufferSize;
        textureParameters = std::move(sourceData.textureParameters);
        cullMode = sourceData.cullMode;
        updateInstances(*this, instances);
    }
    catch (std::exception const& e) {
        Logger::Error("Failed to compile material: ", e.what());
        Logger::Trace("Resorting to default material");
        rgMainPass =
            createShadingPipeline(device, device.createFunction("rgMainPassVS"),
                                  device.createFunction("rgMainPassFS"));
        primitiveParameters = {};
        paramBufferSize = 0;
        textureParameters = {};
        updateInstances(*this, instances);
    }
    depthStencil = device.createDepthStencil(
        { .depthWrite = true,
          .depthCompareFunction = CompareFunction::LessEqual });
}
