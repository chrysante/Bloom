#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/CubemapRenderer.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Texture.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Messages.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct IBLGenTool: View {
    void display();

    void generateIrradiance();
    void extendLifetime(TextureHandle tex) {
        dispatch(DispatchToken::Now, ExtendRenderLifetime{ std::move(tex) });
    }

    Reference<Texture2D> source;
    TextureHandle sourceView;
    Reference<Texture2D> dest;
    TextureHandle destView;
    int sourceside = 0;
    int sourceMipLevel = 0;
    int destside = 0;
};

} // namespace

POPPY_REGISTER_VIEW(IBLGenTool, "IBL Generator");

TextureHandle makeView(Texture2D* tex, int side, int sourceMipLevel = 0) {
    if (!tex || !tex->GPUHandle) {
        return {};
    }
    return createSharedTextureView(RenderAPI::Metal, tex->GPUHandle,
                                   TextureType::Texture2D, tex->pixelFormat,
                                   sourceMipLevel, 1, side, 1);
}

static std::pair<Reference<Texture2D>, TextureHandle> getTexture(
    AssetHandle asset, Emitter& e, int side) {
    if (asset.type() != AssetType::Texture2D) {
        return {};
    }
    Reference<Asset> ref;
    e.dispatch(DispatchToken::Now,
               AMGetAsset{ asset, &ref, AssetRepresentation::GPU });
    auto tex = as<Texture2D>(ref);
    if (!tex || !tex->GPUHandle) {
        return { tex, {} };
    }
    return { tex, makeView(tex.get(), side) };
}

void IBLGenTool::display() {
    static float4 fillColor = { 0, 0, 0, 1 };
    ImGui::ColorPicker4("Fill Color", fillColor.data());
    static bool fillUVs = false;
    ImGui::Checkbox("Fill UVs", &fillUVs);
    if (source && ImGui::Button("Fill Face")) {
        auto& AM = editor().coreSystems().assetManager();
        AM.makeAvailable(source->handle(), AssetRepresentation::CPU);
        float4* data = (float4*)source->data.data();
        size_t imgStride = source->size.x * source->size.y;
        if (!fillUVs) {
            std::fill(data + imgStride * sourceside,
                      data + imgStride * (sourceside + 1), fillColor);
        }
        else {
            data += imgStride * sourceside;
            for (size_t y = 0; y < source->size.y; ++y) {
                for (size_t x = 0; x < source->size.x; ++x) {
                    data[y * source->size.x + x] =
                        float4((float)x / source->size.x,
                               (float)y / source->size.y, 0, 1);
                }
            }
        }
        AM.makeAvailable(source->handle(), AssetRepresentation::GPU, true);
    }
    if (source && source->GPUHandle && ImGui::Button("Generate Mipmaps")) {
        usize2 size = source->size;
        size_t expectedMipapCount =
            vml::ceil(vml::log2((float2)size)).fold(utl::max);
        auto& device = editor().device();
        auto& commandQueue = editor().commandQueue();
        auto commandBuffer = commandQueue.createCommandBuffer();
        auto encoder = commandBuffer->makeBlitEncoder();
        if (source->GPUHandle.description().mipmapLevelCount <
            expectedMipapCount)
        {
            auto desc = source->GPUHandle.description();
            desc.mipmapLevelCount = expectedMipapCount;
            auto newHandle = device.createTexture(desc);
            encoder->copyTexture(source->GPUHandle, newHandle);
            source->GPUHandle = newHandle;
        }
        encoder->generateMipmaps(source->GPUHandle);
        encoder->end();
        commandBuffer->commit();
        commandBuffer->waitForCompletion();
    }
    if (ImGui::SliderInt("Source Side", &sourceside, 0, 5) ||
        (source && source->GPUHandle &&
         ImGui::SliderInt("Source Mip Level", &sourceMipLevel, 0,
                          source->GPUHandle.description().mipmapLevelCount -
                              1)))
    {
        extendLifetime(sourceView);
        sourceView = makeView(source.get(), sourceside, sourceMipLevel);
    }
    ImGui::ImageButton(sourceView.nativeHandle(), { 400, 400 });
    if (auto asset = acceptAssetDragDrop({ AssetType::Texture2D })) {
        extendLifetime(sourceView);
        std::tie(source, sourceView) = getTexture(*asset, *this, sourceside);
    }
    if (ImGui::SliderInt("Dest Side", &destside, 0, 5)) {
        extendLifetime(destView);
        destView = makeView(dest.get(), destside);
    }
    ImGui::ImageButton(destView.nativeHandle(), { 400, 400 });
    if (auto asset = acceptAssetDragDrop({ AssetType::Texture2D })) {
        extendLifetime(destView);
        std::tie(dest, destView) = getTexture(*asset, *this, destside);
    }
    if (ImGui::Button("Generate Irradiance Map")) {
        generateIrradiance();
    }
}

void IBLGenTool::generateIrradiance() {
    if (!source || !source->GPUHandle) {
        Logger::Error("Cannot generate map: No source");
        return;
    }
    if (!dest) {
        Logger::Error("Cannot generate map: No dest");
        return;
    }
    usize2 size = source->size;
    auto& device = editor().device();
    auto& commandQueue = editor().commandQueue();
    auto destHandle =
        device.createTexture({ .type = TextureType::CubeTexture,
                               .pixelFormat = PixelFormat::RGBA32Float,
                               .size = { size, 1 },
                               .usage = TextureUsage::RenderTarget,
                               .storageMode = StorageMode::Shared });
    auto commandBuffer = commandQueue.createCommandBuffer();
    renderIrradianceMap(device, *commandBuffer, destHandle, source->GPUHandle);
    commandBuffer->commit();
    dest->type = TextureType::CubeTexture;
    dest->pixelFormat = source->pixelFormat;
    dest->size = size;
    dest->GPUHandle = destHandle;
    extendLifetime(destView);
    destView = makeView(dest.get(), destside);
}
