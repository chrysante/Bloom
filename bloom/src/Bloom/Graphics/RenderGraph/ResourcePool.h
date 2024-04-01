#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H

#include <vector>

#include "Bloom/Core/Dyncast.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Graphics/RenderGraph/ResourceNodes.h"

namespace bloom::rg {

/// Used by the render graph to allocate resources
class ResourcePool {
public:
    template <typename ResourceType>
    ResourceType* allocate() {
        resources.push_back(bloom::allocate<ResourceType>());
        return static_cast<ResourceType*>(resources.back().get());
    }

private:
    std::vector<DynUniquePtr<Resource>> resources;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
