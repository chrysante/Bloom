#include <imgui.h>
#include <imgui_internal.h>

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
    void displayFaceFillTool();
    void generateIrradiance();
    void generateSH();
    std::pair<Reference<Texture2D>, TextureHandle> getTexture(AssetHandle asset,
                                                              int side);
    void imgDragDropButton(Reference<Texture2D>& tex, TextureHandle& view,
                           int side);
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
    bool showFaceFillTool = false;
    bool applyWindowing = true;
    std::array<float3, 9> SHCoeffs;
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

std::pair<Reference<Texture2D>, TextureHandle> IBLGenTool::getTexture(
    AssetHandle asset, int side) {
    if (asset.type() != AssetType::Texture2D) {
        return {};
    }
    Reference<Asset> ref;
    dispatch(DispatchToken::Now,
             AMGetAsset{ asset, &ref, AssetRepresentation::GPU });
    auto tex = as<Texture2D>(ref);
    if (!tex || !tex->GPUHandle) {
        return { tex, {} };
    }
    return { tex, makeView(tex.get(), side) };
}

void IBLGenTool::imgDragDropButton(Reference<Texture2D>& tex,
                                   TextureHandle& view, int side) {
    ImGui::ImageButton(view.nativeHandle(), { 400, 400 });
    if (auto asset = acceptAssetDragDrop(AssetType::Texture2D)) {
        extendLifetime(view);
        std::tie(tex, view) = getTexture(*asset, side);
    }
}

void IBLGenTool::display() {
    ImGui::Checkbox("Show Face Fill Tool", &showFaceFillTool);
    if (showFaceFillTool) {
        displayFaceFillTool();
    }
    ImGui::SeparatorText("Source");
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
    imgDragDropButton(source, sourceView, sourceside);
    ImGui::Checkbox("Apply Windowing", &applyWindowing);
    if (ImGui::Button("Generate Irradiance Coefficients")) {
        generateSH();
    }
    ImGui::SeparatorText("Dest");
    imgDragDropButton(dest, destView, destside);
    if (dest) {
        for (auto value: dest->irradianceSHCoefficients) {
            ImGui::TextEx(utl::strcat(value).c_str());
        }
        if (ImGui::Button("Copy")) {
            std::stringstream sstr;
            for (auto value: dest->irradianceSHCoefficients)
                sstr << "float3" << value << ",\n";
            ImGui::SetClipboardText(sstr.str().c_str());
        }
    }
}

void IBLGenTool::generateSH() {
    if (source) {
        auto& AM = editor().coreSystems().assetManager();
        AM.makeAvailable(source->handle(), AssetRepresentation::CPU);
    }
    if (!source || !source->pixelData || !dest) {
        Logger::Error("Cannot generate coefficients: No source or dest data");
        return;
    }
    auto* sourceData = reinterpret_cast<float4 const*>(source->pixelData.get());
    auto coeffs =
        genLightingCoefficients(sourceData, source->size, applyWindowing);
    if (!dest->pixelData) {
        dest->pixelData =
            std::make_unique<uint8_t[]>(dest->getPixelDataByteSize());
    }
    dest->irradianceSHCoefficients = coeffs | ranges::to<std::vector>;
}

void IBLGenTool::displayFaceFillTool() {
    static float4 fillColor = { 0, 0, 0, 1 };
    ImGui::ColorPicker4("Fill Color", fillColor.data(),
                        ImGuiColorEditFlags_HDR);
    static bool fillUVs = false;
    ImGui::Checkbox("Fill UVs", &fillUVs);
    if (source && ImGui::Button("Fill Face")) {
        auto& AM = editor().coreSystems().assetManager();
        AM.makeAvailable(source->handle(), AssetRepresentation::CPU);
        float4* data = (float4*)source->pixelData.get();
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
}
