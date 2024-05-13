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
#include "Poppy/Views/TextureViewer.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct TextureViewerWrapper: View {
    TextureViewerWrapper();
    void display() override;
    void doOpenData(std::any data) override;

    TextureViewer impl;
};

} // namespace

POPPY_REGISTER_VIEW(TextureViewerWrapper, "Texture Viewer");

struct TextureViewer::Data {
    Data(Reference<Texture2D> texture, Options options):
        texture(texture), options(options) {}

    TextureView getTextureView();

    void displayOverlay();

    Reference<Texture2D> texture;
    TextureHandle viewHandle;
    TextureHandle
        lastViewHandle; // Really crappy way to extend the lifetime of the
                        // texture to prevent crash in imgui renderer
    int face = 0;
    Options options;
    SplitView splitView;
};

TextureViewer::TextureViewer(): TextureViewer(nullptr, {}) {}
TextureViewer::TextureViewer(Reference<Texture2D> texture, Options options):
    data(std::make_unique<Data>(texture, options)) {}

TextureViewer::TextureViewer(TextureViewer&&) noexcept = default;
TextureViewer& TextureViewer::operator=(TextureViewer&&) noexcept = default;
TextureViewer::~TextureViewer() = default;

static void displayTexture(TextureView texture, float2 availSize,
                           float rounding) {
    if (texture.description().type != TextureType::Texture2D) {
        displayEmptyWithError("Texture not viewable");
        return;
    }
    auto* native = texture.nativeHandle();
    if (!native) {
        displayEmptyWithError("Texture not in GPU memory");
        return;
    }
    float2 pos = ImGui::GetCursorScreenPos();
    float minSize = std::min(availSize.x, availSize.y);
    // FIXME: We assume an aspect ratio of 1 here
    float2 displaySize = { minSize, minSize };
    auto* DL = ImGui::GetWindowDrawList();
    float2 displayPos = pos + (availSize - displaySize) / 2;
    ImRect BB(displayPos, displayPos + displaySize);
    DL->AddImageRounded(native, BB.Min, BB.Max, { 0, 0 }, { 1, 1 },
                        IM_COL32_WHITE, rounding);
    ImGui::ItemAdd(BB, ImGui::GetID("Texture Preview"));
    ImGui::ItemSize(BB.GetSize());
    if (ImGui::IsKeyDown(ImGuiKey_ModAlt) && ImGui::IsItemHovered()) {
        float2 coord = (float2(ImGui::GetMousePos()) - displayPos) /
                       displaySize * texture.size().xy;
        vml::vector<uint8_t, 4> rgba{};
        vml::AABB<uint32_t, 3> region = { uint3(coord, 0),
                                          uint3(coord, 0) + uint3(1) };
        copyTextureToMemory(RenderAPI::Metal, texture, &rgba, 4, region, 0);
        float4 color =
            rgba.map([](uint8_t value) { return float(value) / 255; });
        ImGui::ColorTooltip(utl::strcat("X: ", coord.x, " Y: ", coord.y,
                                        "###Color")
                                .c_str(),
                            color.data(), ImGuiColorEditFlags_Float);
    }
}

static void displayInfo(Texture2D const& texture) {
    using namespace propertiesView;
    if (beginSection("Info")) {
        beginProperty("Type");
        auto typeName = magic_enum::enum_name(texture.type);
        ImGui::TextEx(std::to_address(typeName.begin()),
                      std::to_address(typeName.end()));
        beginProperty("Pixel Format");
        auto pixelFmt = magic_enum::enum_name(texture.pixelFormat);
        ImGui::TextEx(std::to_address(pixelFmt.begin()),
                      std::to_address(pixelFmt.end()));
        beginProperty("Width");
        ImGui::Text("%d", (int)texture.size.x);
        beginProperty("Height");
        ImGui::Text("%d", (int)texture.size.y);
        beginProperty("Byte Size");
        ImGui::Text("%d", (int)texture.data.size());
        endSection();
    }
}

TextureView TextureViewer::Data::getTextureView() {
    if (viewHandle) {
        return viewHandle;
    }
    auto type = texture->type;
    switch (type) {
    case TextureType::Texture2D:
        viewHandle = texture->GPUHandle;
        break;
    case TextureType::CubeTexture:
        viewHandle =
            createSharedTextureView(RenderAPI::Metal, texture->GPUHandle,
                                    TextureType::Texture2D,
                                    texture->pixelFormat, 0, 1, face, 1);
        break;
    default:
        Logger::Error("Cannot display texture of type ",
                      magic_enum::enum_name(type));
        return {};
    }
    return viewHandle;
}

void TextureViewer::Data::displayOverlay() {
    float2 stashedCursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({});
    if (texture->type == TextureType::CubeTexture) {
        char const* names[6] = { "1", "2", "3", "4", "5", "6" };
        if (ImGui::Combo("Face", &face, names, 6)) {
            lastViewHandle = viewHandle;
            viewHandle = {};
        }
    }
    ImGui::SetCursorPos(stashedCursorPos);
}

void TextureViewer::display() {
    if (!data->texture.get()) {
        displayEmptyWithReason("No Texture Opened");
        return;
    }
    TextureView texview = data->getTextureView();
    switch (data->options.displayMode) {
    case DisplayMode::Expanded:
        // clang-format off
        data->splitView.display({
            [&] {
                displayTexture(texview, ImGui::GetContentRegionAvail(), 
                               data->options.rounding);
                data->displayOverlay();
            },
            [&] { displayInfo(*data->texture); },
        }); // clang-format on
        break;
    case DisplayMode::Compact:
        float2 size = ImGui::GetContentRegionAvail();
        displayTexture(texview, float2(size.fold(utl::min)),
                       data->options.rounding);
        data->displayOverlay();
        ImGui::Separator();
        displayInfo(*data->texture);
        break;
    }
}

Texture2D const* TextureViewer::getTexture() { return data->texture.get(); }

TextureViewerWrapper::TextureViewerWrapper() {
    // clang-format off
    setToolbar({
        ToolbarTextItem()
            .text([this] {
                auto* tex = impl.getTexture();
                return tex ? tex->name().c_str() : "";
            })
            .font(FontDesc::UIDefault().setWeight(FontWeight::Bold))
    }); // clang-format on
    setPadding(float2{});
}

void TextureViewerWrapper::display() { impl.display(); }

void TextureViewerWrapper::doOpenData(std::any data) {
    auto* texptr = std::any_cast<Reference<Texture2D>>(&data);
    if (!texptr) {
        return;
    }
    Reference<Texture2D> texture = *texptr;
    auto& AM = editor().coreSystems().assetManager();
    AM.makeAvailable(texture->handle(), AssetRepresentation::GPU);
    impl = TextureViewer(texture, { .displayMode =
                                        TextureViewer::DisplayMode::Expanded });
}
