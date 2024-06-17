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
#include "Poppy/Views/AssetEditorView.h"
#include "Poppy/Views/TextureViewer.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct TextureViewerWrapper: AssetEditorView<Texture2D> {
    TextureViewerWrapper();
    void display() override;
    void openAsset(Texture2D& tex) override;

    TextureViewer impl;
};

} // namespace

POPPY_REGISTER_VIEW(TextureViewerWrapper, "Texture Viewer");

struct TextureViewer::Data {
    Data(Reference<Texture2D> texture, Options options):
        texture(texture), options(options) {}

    TextureView getTextureView();

    void displayOverlay();

    void displayInfo();

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

float4 poppy::copyPixelColor(TextureView texture, float2 coord) {
    auto format = texture.description().pixelFormat;
    size_t numBytes = pixelByteSize(format);
    utl::small_vector<uint8_t> bytes(numBytes);
    if (uint(coord.x) >= texture.size().x || uint(coord.y) >= texture.size().y)
    {
        return {};
    }
    vml::AABB<uint32_t, 3> region = { uint3(coord, 0),
                                      uint3(coord, 0) + uint3(1) };
    copyTextureToMemory(RenderAPI::Metal, texture, bytes.data(), bytes.size(),
                        region, 0);
    switch (format) {
    case PixelFormat::RGBA8Unorm: {
        vml::vector<uint8_t, 4> rgba{};
        std::memcpy(rgba.data(), bytes.data(), sizeof rgba);
        return rgba.map([](uint8_t value) { return float(value) / 255; });
    }
    case PixelFormat::RGBA32Float: {
        float4 color;
        std::memcpy(color.data(), bytes.data(), sizeof color);
        return color;
    }
    default:
        return {};
    }
}

void poppy::displayTextureColorTooltip(
    TextureView texture, float2 coord,
    std::optional<vml::float2> displayCoord) {
    coord = vml::floor(coord);
    float4 color = copyPixelColor(texture, coord);
    coord = displayCoord.value_or(coord);
    auto label = utl::strcat("X: ", coord.x, " Y: ", coord.y, "###Color");
    ImGui::ColorTooltip(label.c_str(), color.data(), ImGuiColorEditFlags_Float);
}

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
    auto ID = ImGui::GetID("Texture Preview");
    ImGui::ItemAdd(BB, ID);
    ImGui::ItemSize(BB.GetSize());
    ImGui::ItemHoverable(BB, ID, ImGuiItemFlags_NoNav);
    if (ImGui::IsKeyDown(ImGuiMod_Alt) && ImGui::IsItemHovered()) {
        float2 coord = (float2(ImGui::GetMousePos()) - displayPos) /
                       displaySize * texture.size().xy;
        displayTextureColorTooltip(texture, coord);
    }
}

void TextureViewer::Data::displayInfo() {
    using namespace propertiesView;
    if (beginSection("Info")) {
        beginProperty("Type");
        auto typeName = magic_enum::enum_name(texture->type);
        ImGui::TextEx(std::to_address(typeName.begin()),
                      std::to_address(typeName.end()));
        beginProperty("Pixel Format");
        auto pixelFmt = magic_enum::enum_name(texture->pixelFormat);
        ImGui::TextEx(std::to_address(pixelFmt.begin()),
                      std::to_address(pixelFmt.end()));
        beginProperty("Width");
        ImGui::Text("%d", (int)texture->size.x);
        beginProperty("Height");
        ImGui::Text("%d", (int)texture->size.y);
        beginProperty("Byte Size");
        ImGui::Text("%d", (int)texture->getPixelDataByteSize());
        endSection();
    }
    if (options.editMode && beginSection("Properties")) {
        beginProperty("Mipmapped");
        bool hasMipmaps = texture->numMipMaps > 1;
        if (ImGui::Checkbox("###Mipmapped", &hasMipmaps)) {
            texture->numMipMaps =
                hasMipmaps ? calcDesiredMipMapCount(texture->size) : 1;
            auto& AM = Application::StaticInstance().assetManager();
            AM.saveToDisk(texture->handle());
            using enum AssetRepresentation;
            AM.makeAvailable(texture->handle(), CPU | GPU, true);
        }
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
    ImGui::SetCursorPos({});
    if (texture->type == TextureType::CubeTexture) {
        char const* names[6] = { "1", "2", "3", "4", "5", "6" };
        if (ImGui::Combo("Face", &face, names, 6)) {
            lastViewHandle = viewHandle;
            viewHandle = {};
        }
    }
}

void TextureViewer::display() {
    if (!data->texture.get()) {
        displayEmptyWithReason("No Texture Opened");
        return;
    }
    TextureView texview = data->getTextureView();
    switch (data->options.displayMode) {
    case DisplayMode::Expanded:
        data->splitView.submit([&] {
            displayTexture(texview, ImGui::GetContentRegionAvail(),
                           data->options.rounding);
            data->displayOverlay();
        });
        data->splitView.submit({ .width = 200,
                                 .flags = ImGuiTableColumnFlags_WidthFixed },
                               [&] {
            ImGui::BeginChild("Info");
            data->displayInfo();
            ImGui::EndChild();
        });
        data->splitView.display();
        break;
    case DisplayMode::Compact:
        float size = ImGui::GetContentRegionAvail().x;
        auto* window = ImGui::GetCurrentWindowRead();
        if (!window->ScrollbarY) {
            size -= ImGui::GetStyle().ScrollbarSize;
        }
        displayTexture(texview, float2(size), data->options.rounding);
        float2 stashedCursorPos = ImGui::GetCursorPos();
        data->displayOverlay();
        ImGui::SetCursorPos(stashedCursorPos);
        ImGui::Separator();
        data->displayInfo();
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

void TextureViewerWrapper::openAsset(Texture2D& tex) {
    auto& AM = editor().assetManager();
    AM.makeAvailable(tex.handle(), AssetRepresentation::GPU);
    impl = TextureViewer(assetRef(),
                         { .displayMode = TextureViewer::DisplayMode::Expanded,
                           .editMode = true });
}
