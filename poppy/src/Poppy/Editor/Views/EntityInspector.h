#ifndef POPPY_EDITOR_VIEWS_ENTITYINSPECTOR_H
#define POPPY_EDITOR_VIEWS_ENTITYINSPECTOR_H

#include "Bloom/Graphics/Lights.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"

namespace poppy {

class EntityInspector: public View, public BasicSceneInspector {
public:
    EntityInspector();

private:
    void frame() override;

    // Component Inspectors
    void inspectTag(bloom::EntityHandle);
    template <typename... Components>
    void addComponentButton(char const* name, bloom::EntityHandle entity,
                            bool forceDisable = false);

    void inspectTransform(bloom::EntityHandle);

    void inspectMesh(bloom::EntityHandle);
    void receiveMeshDragDrop(bloom::EntityHandle);
    void receiveMaterialDragDrop(bloom::EntityHandle);

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

    int editingNameState = 0;
    bool transformScaleLinked = false;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_ENTITYINSPECTOR_H
