#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H

#include <memory>
#include <vector>

#include "Bloom/Graphics/RenderGraph/ResourceNodes.h"

namespace bloom::rg {

/// Used by the render graph to allocate resources
class ResourcePool {
public:
private:
    std::vector<std::unique_ptr<Resource>> resources;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
