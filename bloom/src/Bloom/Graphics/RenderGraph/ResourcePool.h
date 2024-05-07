#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H

#include <vector>

#include "Bloom/Core/Dyncast.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"

namespace bloom {

/// Used by the render graph to allocate resources
class ResourcePool {
public:
    Resource* allocate(InterfaceUnion const& interface);

    /// \Returns a view over the allocated resources
    auto resources() { return _resources | ToAddress<>; }

    /// \overload
    auto resources() const { return _resources | ToAddress<Resource const>; }

    /// Destroys all resources
    void clear() { _resources.clear(); }

private:
    std::vector<DynUniquePtr<Resource>> _resources;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
