#ifndef BLOOM_GRAPHICS_RENDERGRAPH_SCENE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_SCENE_H

#include <vector>

#include <utl/structure_of_arrays.hpp>
#include <vml/vml.hpp>

#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/Lights.h"
#include "Bloom/Graphics/RenderGraph/Node.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/StaticMesh.h"

namespace bloom::rg {

///
UTL_SOA_TYPE(Geometry, (Reference<StaticMeshRenderer>, mesh),
             (Reference<MaterialInstance>, materialInstance),
             (vml::float4x4, transform));

/// Geometry to be passed to the renderer. This is rebuilt every frame
class SceneGeometry {
public:
    void add(Reference<StaticMeshRenderer> mesh,
             Reference<MaterialInstance> material,
             vml::float4x4 const& transform);

    ///
    void clear();

    ///
    void prepare();

    ///
    utl::structure_of_arrays<Geometry> const& getObjects() const {
        return objects;
    }

    ///
    BufferHandle const& transformBuffer() const { return transforms; }

private:
    utl::structure_of_arrays<Geometry> objects;

    BufferHandle transforms;
};

///
class SceneLights {
public:
    void add(PointLight const& light);
    void add(SpotLight const& light);
    void add(DirectionalLight const& light);
    void add(SkyLight const& light);

    ///
    void clear();

    ///
    void prepare();

    ///
    BufferHandle const& buffer() const { return _buffer; }

private:
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<DirectionalLight> dirLights;
    std::vector<SkyLight> skyLights;

    BufferHandle _buffer;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_SCENE_H
