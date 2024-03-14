#include "Poppy/Editor/Views/MaterialInstanceViewer.hpp"

#include <imgui.h>

#include "Bloom/Asset/AssetManager.hpp"
#include "Bloom/Graphics/Material/MaterialInstance.hpp"
#include "Poppy/Editor/Editor.hpp"
#include "Poppy/Editor/Views/AssetBrowser.hpp"
#include "Poppy/UI/PropertiesView.hpp"

using namespace bloom;
using namespace poppy;

POPPY_REGISTER_VIEW(MaterialInstanceViewer, "Material Instance Viewer",
                    { .persistent = false, .unique = false });

void MaterialInstanceViewer::frame() {
    if (!inst) {
        displayEmptyWithReason("No Material Instance opened");
        return;
    }
    auto materialName = inst->material() ? inst->material()->name() :
                                           "No Material Assigned";
    ImGui::Button(materialName.data(),
                  { ImGui::GetContentRegionAvail().x, 30 });
    if (auto const assetHandle = acceptAssetDragDrop({ AssetType::Material })) {
        auto& assetManager = editor().coreSystems().assetManager();
        auto mat = as<Material>(assetManager.get(*assetHandle));
        assetManager.makeAvailable(*assetHandle, AssetRepresentation::GPU);
        inst->setMaterial(std::move(mat));
        assetManager.saveToDisk(inst->handle());
    }

    using namespace propertiesView;

    auto params = inst->paramaters();
    bool edited = false;
    header("Parameters");
    if (beginSection()) {
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

void MaterialInstanceViewer::setMaterialInstance(
    Reference<MaterialInstance> _) {
    inst = std::move(_);
    editor()
        .coreSystems()
        .assetManager()
        .makeAvailable(inst->handle(),
                       AssetRepresentation::CPU | AssetRepresentation::GPU);
    setTitle(std::string(inst->name()));
}
