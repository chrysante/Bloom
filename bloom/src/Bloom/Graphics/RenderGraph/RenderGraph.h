#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H

#include <memory>
#include <span>
#include <vector>

#include <utl/hashtable.hpp>
#include <utl/vector.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"

namespace bloom::rg {

///
class RenderGraph {
public:
    explicit RenderGraph(HardwareDevice* device);

    ///
    Pass* addPass(std::unique_ptr<Pass> pass);

    ///
    template <std::derived_from<Pass> P>
    P* addPass(std::unique_ptr<P> pass) {
        auto* p = addPass(std::unique_ptr<Pass>(pass.release()));
        return static_cast<P*>(p);
    }

    ///
    template <InterfaceType Interface>
    auto* declareInput(Interface const& interface) {
        using R = typename ToResourceType<Interface>::type;
        inputs.push_back(allocate<R>(interface, LinkType::Output, nullptr));
        return cast<R*>(inputs.back().get());
    }

    ///
    void addLink(Resource* from, Resource* to);

    ///
    bool compile();

    /// Executes the graph
    void execute();

    Pass* findPass(std::string_view name);

    Resource* findInputResource(std::string_view name);

private:
    HardwareDevice* device;
    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<DynUniquePtr<Resource>> inputs;
    std::vector<Pass*> executionOrder;
};

std::unique_ptr<RenderGraph> createDefaultRenderGraph(HardwareDevice* device);

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
