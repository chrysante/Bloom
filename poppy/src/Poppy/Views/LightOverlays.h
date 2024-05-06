#ifndef POPPY_VIEWS_LIGHTOVERLAYS_H
#define POPPY_VIEWS_LIGHTOVERLAYS_H

#include <span>

#include <utl/function_view.hpp>

#include "Bloom/Asset/Fwd.h"
#include "Poppy/Editor/EntitySelectionManager.h"

namespace poppy {

/// \Returns the clicked entity if any
bloom::EntityHandle displayLightOverlays(
    std::span<bloom::Scene* const> scenes,
    EntitySelectionManager const& selection,
    utl::function_view<vml::float3(vml::float3)> WStoSS);

} // namespace poppy

#endif // POPPY_VIEWS_LIGHTOVERLAYS_H
