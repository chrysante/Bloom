#include <imgui.h>

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
    auto params = inst->paramaters();
    bool edited = false;
    if (beginSection("Parameters")) {
        beginProperty("Base Color");
        edited |= ImGui::ColorEdit3("##base-color", params.baseColor.data(),
                                    ImGuiColorEditFlags_NoInputs |
                                        ImGuiColorEditFlags_NoLabel |
                                        ImGuiColorEditFlags_Float |
                                        ImGuiColorEditFlags_NoAlpha |
                                        ImGuiColorEditFlags_PickerHueWheel);
        beginProperty("Metallic");
        fullWidth();
        edited |= ImGui::SliderFloat("##metallic", &params.metallic, 0, 1);
        beginProperty("Roughness");
        fullWidth();
        edited |= ImGui::SliderFloat("##roughness", &params.roughness, 0, 1);

        beginProperty("Emissive");
        edited |= ImGui::ColorEdit3("##emissive", params.emissive.data(),
                                    ImGuiColorEditFlags_NoInputs |
                                        ImGuiColorEditFlags_NoLabel |
                                        ImGuiColorEditFlags_Float |
                                        ImGuiColorEditFlags_NoAlpha |
                                        ImGuiColorEditFlags_PickerHueWheel);
        beginProperty("Emissive Intensity");
        fullWidth();
        edited |= ImGui::DragFloat("##emissive-intensity",
                                   &params.emissiveIntensity, .1);

        endSection();
    }
    if (edited) {
        inst->setParamaters(params);
    }
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
