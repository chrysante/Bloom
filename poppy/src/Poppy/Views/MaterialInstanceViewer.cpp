#include <imgui.h>
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

namespace {

struct MaterialInstanceViewer: public View {
    MaterialInstanceViewer();

    void display() override;

    void receiveData(std::any data) override;

    bloom::Reference<bloom::MaterialInstance> inst;
};

} // namespace

POPPY_REGISTER_VIEW(MaterialInstanceViewer, "Material Instance Viewer");

MaterialInstanceViewer::MaterialInstanceViewer() {
    // clang-format off
    setToolbar({
        ToolbarSpacer(),
        ToolbarTextItem()
            .text([this] { return inst ? inst->name().c_str() : ""; })
            .font(FontDesc::UIDefault().setWeight(FontWeight::Semibold)),
        ToolbarSpacer(),
    }); // clang-format on
}

static std::string makeLabel(std::string_view name) {
    return utl::strcat("###", name);
}

static bool displayParam(MaterialPrimitiveParameter& param) {
    using namespace propertiesView;
    beginProperty(param.name);
    std::string label = makeLabel(param.name);
    using enum MaterialDataType;
    switch (param.dataType) {
    case Float:
        fullWidth();
        return ImGui::DragFloat(label.c_str(), param.defaultValue.data());
    case Float2:
        fullWidth();
        return ImGui::DragFloat2(label.c_str(), param.defaultValue.data());
    case Float3:
        return ImGui::ColorEdit3(label.c_str(), param.defaultValue.data(),
                                 ImGuiColorEditFlags_NoInputs |
                                     ImGuiColorEditFlags_NoLabel |
                                     ImGuiColorEditFlags_Float |
                                     ImGuiColorEditFlags_NoAlpha);
    case Float4:
        return ImGui::ColorEdit4(label.c_str(), param.defaultValue.data(),
                                 ImGuiColorEditFlags_NoInputs |
                                     ImGuiColorEditFlags_NoLabel |
                                     ImGuiColorEditFlags_Float);
    default:
        return false;
    }
}

static void displayParam(MaterialInstance& inst,
                         MaterialTextureParameter& param) {
    using namespace propertiesView;
    beginProperty(param.name);
    auto tex =
        inst.getTexture(param.vertexBindingPoint, param.fragmentBindingPoint);
    using enum AssetRepresentation;
    if (assetButton(makeLabel(param.name).c_str(), tex,
                    { .style = AssetButtonStyle::Thumbnail, .repr = GPU }))
    {
        param.defaultValue = tex->handle();
        inst.setTexture(param.vertexBindingPoint, param.fragmentBindingPoint,
                        std::move(tex));
    }
}

void MaterialInstanceViewer::display() {
    if (!inst) {
        displayEmptyWithReason("No Material Instance Open");
        return;
    }
    using namespace propertiesView;
    if (beginSection()) {
        beginProperty("Material");
        auto material = inst->materialRef();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (assetButton("###Material", material,
                        { .repr = AssetRepresentation::GPU }))
        {
            inst->setMaterial(std::move(material));
        }
        endSection();
    }
    auto primParams = inst->getPrimitiveParameterDescriptors();
    auto texParams = inst->getTextureParameterDescriptors();
    if (primParams.empty() && texParams.empty()) {
        return;
    }
    if (!beginSection("Parameters")) {
        return;
    }
    for (auto& param: primParams) {
        if (displayParam(param)) {
            inst->setParameter(param.defaultValue.data(), param.bufferOffset,
                               layoutSize(param.dataType));
        }
    }
    for (auto& param: texParams) {
        displayParam(*inst, param);
    }
    endSection();
}

void MaterialInstanceViewer::receiveData(std::any data) {
    if (receiveAsset(inst, data)) {
        return;
    }
    auto& AM = editor().coreSystems().assetManager();
    using enum AssetRepresentation;
    AM.makeAvailable(inst->handle(), CPU | GPU);
}
