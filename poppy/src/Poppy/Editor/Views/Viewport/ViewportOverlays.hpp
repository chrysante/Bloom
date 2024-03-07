#pragma once

#include <mtl/mtl.hpp>

#include "Bloom/Scene/Components/Lights.hpp"
#include "Bloom/Scene/Components/Transform.hpp"
#include "Bloom/Scene/Entity.hpp"

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

    void drawPointLightIcon(mtl::float2 position, mtl::float3 color);
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
