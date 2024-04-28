#include "Bloom/Graphics/RenderGraph/ResourcePool.h"

using namespace bloom;
using namespace rg;

template <typename I>
static DynUniquePtr<Resource> allocateResource(I const&) {
    return bloom::allocate<typename I::Resource>();
}

template <typename I>
    requires requires(I const& i) { i.desc(); }
static DynUniquePtr<Resource> allocateResource(I const& interface) {
    return bloom::allocate<typename I::Resource>(interface.desc());
}

Resource* ResourcePool::allocate(InterfaceUnion const& interface) {
    auto res = interface.visit([&]<typename I>(I const& interface) {
        return allocateResource(interface);
    });
    _resources.push_back(std::move(res));
    return _resources.back().get();
}
