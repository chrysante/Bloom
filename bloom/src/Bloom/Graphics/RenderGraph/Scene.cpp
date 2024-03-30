#include "Bloom/Graphics/RenderGraph/Scene.h"

#include "Bloom/Core/Debug.h"

using namespace bloom;
using namespace rg;

void SceneGeometry::add(Reference<StaticMeshRenderer> mesh,
                        Reference<MaterialInstance> material,
                        vml::float4x4 const& transform) {
    objects.emplace_back(std::move(mesh), std::move(material), transform);
}

void SceneGeometry::clear() { objects.clear(); }

void SceneGeometry::prepare() { BL_UNIMPLEMENTED(); }

void SceneLights::add(PointLight const& light) { pointLights.push_back(light); }

void SceneLights::add(SpotLight const& light) { spotLights.push_back(light); }

void SceneLights::add(DirectionalLight const& light) {
    dirLights.push_back(light);
}

void SceneLights::add(SkyLight const& light) { skyLights.push_back(light); }

void SceneLights::clear() {
    pointLights.clear();
    spotLights.clear();
    dirLights.clear();
    skyLights.clear();
}

void SceneLights::prepare() { BL_UNIMPLEMENTED(); }
