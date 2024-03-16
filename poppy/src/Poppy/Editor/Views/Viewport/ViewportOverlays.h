#ifndef POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTOVERLAYS_H
#define POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTOVERLAYS_H

#include <mtl/mtl.hpp>

#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Entity.h"

namespace poppy {

class Viewport;

class ViewportOverlays {
public:
    void init(Viewport*);
    void display();
    bloom::EntityHandle hitTest(mtl::float2 positionInView);

private:
    template <typename>
    void drawLightOverlays();
    template <typename Light>
    void drawOneLightOverlay(bloom::EntityHandle, mtl::float2 positionInWindow,
                             bool selected, bloom::Transform const&,
                             Light const&);

    void drawPointLightIcon(mtl::float2 position, mtl::float3 color,
                            bool selected);
    void drawSpotLightIcon(mtl::float2 position, mtl::float3 color);
    void drawDirectionalLightIcon(mtl::float2 position, mtl::float3 directionWS,
                                  mtl::float3 color);
    void drawSkyLightIcon(mtl::float2 position, mtl::float3 color);
    void drawSpotlightVizWS(bloom::EntityHandle entity, float radius,
                            float angle, mtl::float3 color);

    template <typename Light>
    bloom::EntityHandle hitTestLightOverlays(mtl::float2 hitPositionInView);

    template <typename Light>
    bool hitTestOneLightOverlay(mtl::float2 entityPositionInView,
                                mtl::float2 hitPositionInView);

private:
    Viewport* viewport = nullptr;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTOVERLAYS_H
