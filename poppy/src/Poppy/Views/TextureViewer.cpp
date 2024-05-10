#include <imgui.h>
#include <imgui_internal.h>
#include <magic_enum.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Graphics/Texture.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct TextureViewer: View {
    TextureViewer();
    void display() override;
    void displayTexture();
    void displayInfo();
    void doOpenData(std::any data) override;

    Reference<Texture2D> texture;
    std::optional<float4> hoverColor;
};

} // namespace

POPPY_REGISTER_VIEW(TextureViewer, "Texture Viewer");

TextureViewer::TextureViewer() {
    // clang-format off
    setToolbar({
        ToolbarTextItem()
            .text([this] { return texture ? texture->name().c_str() : ""; })
            .font(FontDesc::UIDefault().setWeight(FontWeight::Bold))
    }); // clang-format on
    setPadding(float2{});
}

void TextureViewer::display() {
    if (!texture.get()) {
        displayEmptyWithReason("No Texture Opened");
        return;
    }
    // clang-format off
    splitView("SplitView", {
        [&] { displayTexture(); },
        [&] { displayInfo(); },
    }); // clang-format on
}

void TextureViewer::displayTexture() {
    BL_ASSERT(texture);
    auto* native = texture->GPUHandle.nativeHandle();
    if (!native) {
        displayEmptyWithError("Texture not in GPU memory");
        return;
    }
    float2 pos = ImGui::GetCursorScreenPos();
    float2 availSize = ImGui::GetContentRegionAvail();
    float minSize = std::min(availSize.x, availSize.y);
    // FIXME: We assume an aspect ratio of 1 here
    float2 displaySize = { minSize, minSize };
    auto* DL = ImGui::GetWindowDrawList();
    float2 displayPos = pos + (availSize - displaySize) / 2;
    ImRect BB(displayPos, displayPos + displaySize);
    DL->AddImage(native, BB.Min, BB.Max);
    ImGui::ItemAdd(BB, ImGui::GetID("Texture Preview"));
    ImGui::ItemSize(BB.GetSize());
    if (ImGui::IsItemHovered()) {
        float2 coord = (float2(ImGui::GetMousePos()) - displayPos) /
                       displaySize * texture->size;
        vml::vector<uint8_t, 4> rgba{};
        vml::AABB<uint32_t, 3> region = { uint3(coord, 0),
                                          uint3(coord, 0) + uint3(1) };
        copyTextureToMemory(RenderAPI::Metal, texture->GPUHandle, &rgba, 4,
                            region, 0);
        float4 color =
            rgba.map([](uint8_t value) { return float(value) / 255; });
        ImGui::ColorTooltip(utl::strcat("X: ", coord.x, " Y: ", coord.y,
                                        "###Color")
                                .c_str(),
                            color.data(), ImGuiColorEditFlags_Float);
    }
}

void TextureViewer::displayInfo() {
    BL_ASSERT(texture);
    using namespace propertiesView;
    if (beginSection("Info")) {
        beginProperty("Pixel Format");
        auto str = magic_enum::enum_name(texture->pixelFormat);
        ImGui::TextEx(std::to_address(str.begin()), std::to_address(str.end()));

        beginProperty("Width");
        ImGui::Text("%d", (int)texture->size.x);
        beginProperty("Height");
        ImGui::Text("%d", (int)texture->size.y);
        beginProperty("Byte Size");
        ImGui::Text("%d", (int)texture->data.size());
        endSection();
    }
}

void TextureViewer::doOpenData(std::any data) {
    auto* tex = std::any_cast<Reference<Texture2D>>(&data);
    if (!tex || !(texture = *tex).get()) {
        return;
    }
    auto& AM = editor().coreSystems().assetManager();
    AM.makeAvailable(texture->handle(), AssetRepresentation::GPU);
}
