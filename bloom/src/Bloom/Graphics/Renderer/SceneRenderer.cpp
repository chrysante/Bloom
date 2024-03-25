#include "Bloom/Graphics/Renderer/SceneRenderer.h"

#include <utl/functional.hpp>

#include "Bloom/Core/Core.h"
#include "Bloom/Graphics/Renderer/Renderer.h"
#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/MeshRenderer.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Scene.h"

using namespace bloom;

void SceneRenderer::submitScenes(std::span<Scene const* const> scenes,
                                 Camera const& camera) {
    assert(mRenderer);
    renderer().beginScene(camera);
    for (auto* scene: scenes) {
        submitScene(*scene);
    }
    submitExtra();
    renderer().endScene();
}

template <typename LightComponent>
static void submitLights(Renderer& renderer, Scene const& scene,
                         auto&& lightModifier) {
    auto view =
        scene.view<TransformMatrixComponent const, LightComponent const>();
    view.each([&](auto /* ID */, TransformMatrixComponent const& transform,
                  LightComponent light) {
        lightModifier(transform, light);
        renderer.submit(light.light);
    });
}

template <typename Light>
static void submitLights(Renderer& renderer, Scene const& scene) {
    submitLights<Light>(renderer, scene, utl::noop);
}

void SceneRenderer::submitScene(Scene const& scene) {
    /* submit meshes */ {
        auto view = scene.view<TransformMatrixComponent const,
                               MeshRendererComponent const>();
        view.each([&](auto /* ID */, TransformMatrixComponent const& transform,
                      MeshRendererComponent const& meshRenderer) {
            if (!meshRenderer.mesh || !meshRenderer.materialInstance ||
                !meshRenderer.materialInstance->material())
            {
                return;
            }
            renderer().submit(meshRenderer.mesh->getRenderer(),
                              meshRenderer.materialInstance, transform.matrix);
        });
    }
    submitLights<PointLightComponent>(renderer(), scene,
                                      [](auto& transform, auto& light) {
        light.light.position = transform.matrix.column(3).xyz;
    });
    submitLights<SpotLightComponent>(renderer(), scene,
                                     [](auto& transform, auto& light) {
        light.light.position = transform.matrix.column(3).xyz;
        light.light.direction =
            vml::normalize((transform.matrix * vml::float4{ 1, 0, 0, 0 }).xyz);
    });
    submitLights<DirectionalLightComponent>(renderer(), scene,
                                            [](auto& transform, auto& light) {
        light.light.direction =
            vml::normalize((transform.matrix * vml::float4{ 0, 0, 1, 0 }).xyz);
    });
    submitLights<SkyLightComponent>(renderer(), scene);
}
