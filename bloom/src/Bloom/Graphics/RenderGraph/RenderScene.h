#ifndef RenderScene_hpp
#define RenderScene_hpp

#include <vector>

#include <utl/structure_of_arrays.hpp>
#include <vml/vml.hpp>

#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/Lights.h"
#include "Bloom/Graphics/RenderGraph/Node.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/StaticMesh.h"

namespace bloom {

///
UTL_SOA_TYPE(RenderObject, (Reference<StaticMeshRenderer>, mesh),
             (Reference<MaterialInstance>, materialInstance),
             (vml::float4x4, transform));

/// Scene to be passed to the renderer. This is rebuilt every frame
class RenderScene: public RGResourceNode {
public:
    RenderScene();

    void setCamera(Camera camera) { this->camera = std::move(camera); }

    void add(Reference<StaticMeshRenderer> mesh,
             Reference<MaterialInstance> material,
             vml::float4x4 const& transform);
    void add(PointLight const& light);
    void add(SpotLight const& light);
    void add(DirectionalLight const& light);
    void add(SkyLight const& light);

    utl::structure_of_arrays<RenderObject> const& getObjects() const {
        return objects;
    }

    BufferHandle const& transformBuffer() const { return transforms; }

private:
    Camera camera;
    utl::structure_of_arrays<RenderObject> objects;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<DirectionalLight> dirLights;
    std::vector<SkyLight> skyLights;

    BufferHandle transforms;
};

} // namespace bloom

#endif /* RenderScene_hpp */
