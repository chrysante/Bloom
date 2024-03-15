#ifndef BLOOM_SCENE_COMPONENTS_MESHRENDERER_H
#define BLOOM_SCENE_COMPONENTS_MESHRENDERER_H

#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

class AssetManager;

struct BLOOM_API MeshRendererComponent {
    BLOOM_REGISTER_COMPONENT("Mesh Renderer")

    Reference<MaterialInstance> materialInstance;
    Reference<StaticMesh> mesh;

    YAML::Node serialize() const;
    void deserialize(YAML::Node const&, AssetManager&);
};

} // namespace bloom

#endif // BLOOM_SCENE_COMPONENTS_MESHRENDERER_H
