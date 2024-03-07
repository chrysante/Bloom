#pragma once

#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Scene/Components/AllComponents.hpp"
#include "Bloom/Scene/Entity.hpp"
#include "Poppy/Editor/Views/BasicSceneInspector.hpp"
#include "Poppy/UI/View.hpp"
#include "PropertiesView.hpp"

namespace poppy {

class EntityInspector: public View, public BasicSceneInspector {
public:
    EntityInspector();

private:
    void frame() override;

    // Component Inspectors
    void inspectTag(bloom::EntityHandle);

    void inspectTransform(bloom::EntityHandle);

    void inspectMesh(bloom::EntityHandle);
    void recieveMeshDragDrop(bloom::EntityHandle);
    void recieveMaterialDragDrop(bloom::EntityHandle);

    void inspectLight(bloom::EntityHandle);
    void inspectLightType(LightType&, bloom::EntityHandle);

    void inspectLightCommon(bloom::LightCommon&, LightType type);
    void inspectPointLight(bloom::PointLight&);
    void inspectSpotLight(bloom::SpotLight&);
    void inspectDirectionalLight(bloom::DirectionalLight&);
    void inspectSkyLight(bloom::SkyLight&);

    void inspectScript(bloom::EntityHandle);

    // Helpers
    bool beginComponentSection(std::string_view name);
    template <typename>
    bool beginComponentSection(std::string_view name, bloom::EntityHandle);
    bool beginSubSection(std::string_view name);

    bool beginGenericSection(std::string_view name, Font const&,
                             utl::function<void()> deleter);

    void endSection();
    void endSubSection();

private:
    int editingNameState = 0;
    bool transformScaleLinked = false;
};

} // namespace poppy
