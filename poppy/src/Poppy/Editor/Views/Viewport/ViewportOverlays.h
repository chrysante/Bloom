#ifndef POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTOVERLAYS_H
#define POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTOVERLAYS_H

#include <vml/vml.hpp>

#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Entity.h"

namespace poppy {

class Viewport;

class ViewportOverlays {
public:
    void init(Viewport*);
    void display();
    bloom::EntityHandle hitTest(vml::float2 positionInView);

private:
    template <typename>
    void drawLightOverlays();
    template <typename Light>
    void drawOneLightOverlay(bloom::EntityHandle, vml::float2 positionInWindow,
                             bool selected, bloom::Transform const&,
                             Light const&);

    void drawPointLightIcon(vml::float2 position, vml::float3 color,
                            bool selected);
    void drawSpotLightIcon(vml::float2 position, vml::float3 color);
    void drawDirectionalLightIcon(vml::float2 position, vml::float3 directionWS,
                                  vml::float3 color);
    void drawSkyLightIcon(vml::float2 position, vml::float3 color);
    void drawSpotlightVizWS(bloom::EntityHandle entity, float radius,
                            float angle, vml::float3 color);

    template <typename Light>
    bloom::EntityHandle hitTestLightOverlays(vml::float2 hitPositionInView);

    template <typename Light>
    bool hitTestOneLightOverlay(vml::float2 entityPositionInView,
                                vml::float2 hitPositionInView);

private:
    Viewport* viewport = nullptr;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTOVERLAYS_H
