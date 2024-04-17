#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H

#include <vector>

#include "Bloom/Core/Dyncast.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"

namespace bloom::rg {

/// Used by the render graph to allocate resources
class ResourcePool {
public:
    template <InterfaceType Type>
    ResourceType<Type>* allocate(InterfaceDesc<Type> desc) {
        _resources.push_back(bloom::allocate<ResourceType<Type>>(desc));
        return static_cast<ResourceType<Type>*>(_resources.back().get());
    }

    /// \Returns a view over the allocated resources
    auto resources() { return _resources | ToAddress<>; }

    /// \overload
    auto resources() const { return _resources | ToAddress<Resource const>; }

    ///
    void clear() { _resources.clear(); }

private:
    std::vector<DynUniquePtr<Resource>> _resources;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCEPOOL_H
