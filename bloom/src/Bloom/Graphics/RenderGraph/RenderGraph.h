#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H

#include <memory>
#include <vector>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Node.h"

namespace bloom::rg {

///
class RenderGraph {
public:
    explicit RenderGraph(HardwareDevice* device);

    ///
    Pass* addPass(std::unique_ptr<Pass> pass);

    ///
    UserResource* addUserResource(std::string name,
                                  ResourceHandleVariant handle);

    ///
    void addLink(Resource* from, PassInput* to);

    ///
    void compile();

    /// Executes the graph
    void execute();

    Pass* findPass(std::string_view name);
    Resource* findResource(std::string_view name);

private:
    HardwareDevice* device;
    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<std::unique_ptr<Resource>> resources;
};

std::unique_ptr<RenderGraph> createDefaultRenderGraph(HardwareDevice* device);

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
