#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Graphics/Texture.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace poppy;
using namespace bloom;

namespace {

struct TextureViewer: View {
    TextureViewer();
    void display() override;
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
}

void TextureViewer::display() {
    if (!texture.get()) {
        displayEmptyWithReason("No Texture Opened");
        return;
    }
    auto* native = texture->GPUHandle.nativeHandle();
    if (!native) {
        displayEmptyWithError("Texture not in GPU memory");
        return;
    }
    ImGui::Image(native, ImGui::GetContentRegionAvail());
}

void TextureViewer::doOpenData(std::any data) {
    auto* tex = std::any_cast<Reference<Texture2D>>(&data);
    if (!tex || !(texture = *tex).get()) {
        return;
    }
    auto& AM = editor().coreSystems().assetManager();
    AM.makeAvailable(texture->handle(), AssetRepresentation::GPU);
}
