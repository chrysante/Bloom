#include "Bloom/Graphics/RenderGraph/RenderScene.h"

using namespace bloom;

RenderScene::RenderScene(): RGResourceNode("Scene") {}

void RenderScene::add(Reference<StaticMeshRenderer> mesh,
                      Reference<MaterialInstance> material,
                      vml::float4x4 const& transform) {
    objects.emplace_back(std::move(mesh), std::move(material), transform);
}

void RenderScene::add(PointLight const& light) { pointLights.push_back(light); }

void RenderScene::add(SpotLight const& light) { spotLights.push_back(light); }

void RenderScene::add(DirectionalLight const& light) {
    dirLights.push_back(light);
}

void RenderScene::add(SkyLight const& light) { skyLights.push_back(light); }
