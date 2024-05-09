#include <imgui.h>
#include <imgui_internal.h>
#include <vml/vml.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Graphics/Texture.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"
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
    void doOpenData(std::any data) override;

    Reference<Texture2D> texture;
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
        [&] {},
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
    DL->AddImage(native, displayPos, displayPos + displaySize);
}

void TextureViewer::doOpenData(std::any data) {
    auto* tex = std::any_cast<Reference<Texture2D>>(&data);
    if (!tex || !(texture = *tex).get()) {
        return;
    }
    auto& AM = editor().coreSystems().assetManager();
    AM.makeAvailable(texture->handle(), AssetRepresentation::GPU);
}
