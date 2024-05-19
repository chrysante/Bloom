#include <imgui.h>
#include <utl/strcat.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Core.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
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

static bool displayParam(MaterialPrimitiveParameter& param) {
    using namespace propertiesView;
    beginProperty(param.name);
    std::string label = utl::strcat("###", param.name);
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
        if (assetButton(material, { .repr = AssetRepresentation::GPU })) {
            inst->setMaterial(std::move(material));
        }
        endSection();
    }
    if (!beginSection("Parameters")) {
        return;
    }
    for (auto& param: inst->getPrimitiveParameterDescriptors()) {
        if (displayParam(param)) {
            inst->setParameter(param.defaultValue.data(), param.bufferOffset,
                               layoutSize(param.dataType));
        }
    }
    endSection();
}

void MaterialInstanceViewer::receiveData(std::any data) {
    auto* newInst = std::any_cast<Reference<MaterialInstance>>(&data);
    if (!newInst) {
        return;
    }
    inst = *newInst;
    auto& AM = editor().coreSystems().assetManager();
    using enum AssetRepresentation;
    AM.makeAvailable(inst->handle(), CPU | GPU);
}
