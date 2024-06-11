#include <imgui.h>
#include <range/v3/view.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Core.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/Texture.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"

using namespace bloom;
using namespace poppy;
using namespace ranges::views;

namespace {

struct MaterialInstanceEditor: public View {
    MaterialInstanceEditor();

    void display() override;

    void receiveData(std::any data) override;

    bloom::Reference<bloom::MaterialInstance> inst;
};

} // namespace

POPPY_REGISTER_VIEW(MaterialInstanceEditor, "Material Instance Editor");

MaterialInstanceEditor::MaterialInstanceEditor() {
    // clang-format off
    setToolbar({
        ToolbarSpacer(),
        ToolbarTextItem()
            .text([this] { return inst ? inst->name().c_str() : ""; })
            .font(FontDesc::UIDefault().setWeight(FontWeight::Semibold)),
        ToolbarSpacer(),
    }); // clang-format on
    setPadding({ 0, 0 });
}

static std::string makeLabel(std::string_view name) {
    return utl::strcat("###", name);
}

static bool displayParam(MaterialPrimitiveParameter& param) {
    using namespace propertiesView;
    beginProperty(param.name);
    std::string label = makeLabel(param.name);
    using enum MaterialDataType;
    auto colorEditFlags = ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
    fullWidth();
    switch (param.type) {
    case Float:
        return ImGui::DragFloat(label.c_str(), param.value.data(), 0.01);
    case Float2:
        return ImGui::DragFloat2(label.c_str(), param.value.data(), 0.01);
    case Float3:
        return ImGui::ColorEdit3(label.c_str(), param.value.data(),
                                 colorEditFlags | ImGuiColorEditFlags_NoAlpha);
    case Float4:
        return ImGui::ColorEdit4(label.c_str(), param.value.data(),
                                 colorEditFlags);
    default:
        return false;
    }
}

static void displayParam(MaterialInstance& inst,
                         MaterialTextureParameter& param, size_t index) {
    using namespace propertiesView;
    beginProperty(param.name);
    auto tex = inst.getTexture(index);
    using enum AssetRepresentation;
    if (assetButton(makeLabel(param.name).c_str(), tex,
                    { .style = AssetButtonStyle::Thumbnail, .repr = GPU }))
    {
        param.value = tex ? tex->handle() : AssetHandle{};
        inst.setTexture(index, std::move(tex));
    }
}

void MaterialInstanceEditor::display() {
    if (!inst) {
        displayEmptyWithReason("No Material Instance Open");
        return;
    }
    ImGui::BeginChild("Child", {}, ImGuiChildFlags_AlwaysUseWindowPadding);
    using namespace propertiesView;
    if (beginSection()) {
        beginProperty("Material");
        auto material = inst->materialRef();
        fullWidth();
        if (assetButton("###Material", material,
                        { .repr = AssetRepresentation::GPU }))
        {
            inst->setMaterial(std::move(material));
        }
        endSection();
    }
    auto primParams = inst->getPrimitiveParameters();
    auto texParams = inst->getTextureParameters();
    if (primParams.empty() && texParams.empty()) {
        ImGui::EndChild();
        return;
    }
    if (!beginSection("Parameters")) {
        ImGui::EndChild();
        return;
    }
    for (auto& param: primParams) {
        if (displayParam(param)) {
            inst->setParameter(param.value.data(), param.bufferOffset,
                               layoutSize(param.type));
        }
    }
    for (auto [index, param]: texParams | enumerate) {
        displayParam(*inst, param, index);
    }
    endSection();
    ImGui::EndChild();
}

void MaterialInstanceEditor::receiveData(std::any data) {
    if (receiveAsset(inst, data)) {
        return;
    }
    auto& AM = editor().assetManager();
    using enum AssetRepresentation;
    AM.makeAvailable(inst->handle(), CPU | GPU);
}
