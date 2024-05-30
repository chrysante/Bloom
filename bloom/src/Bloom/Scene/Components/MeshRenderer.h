#ifndef BLOOM_SCENE_COMPONENTS_MESHRENDERER_H
#define BLOOM_SCENE_COMPONENTS_MESHRENDERER_H

#include "Bloom/Core/Base.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

class AssetManager;

struct BLOOM_API MeshRendererComponent {
    struct WeakType {
        using StrongType = MeshRendererComponent;

        AssetHandle materialInstance;
        AssetHandle mesh;
    };

    MeshRendererComponent() = default;
    MeshRendererComponent(WeakType const& weak);

    operator WeakType() const;

    Reference<MaterialInstance> materialInstance;
    Reference<StaticMesh> mesh;

    YAML::Node serialize() const;
    void deserialize(YAML::Node const&, AssetManager&);
};

} // namespace bloom

#endif // BLOOM_SCENE_COMPONENTS_MESHRENDERER_H
