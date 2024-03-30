#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H

#include <vector>

#include "Bloom/Graphics/RenderGraph/Node.h"

namespace bloom {

///
class RenderGraph {
public:
    ///
    RGPassNode* addPass(std::unique_ptr<RGPassNode> pass);

    ///
    RGResource* addResource(std::unique_ptr<RGResourceNode> node);

    void addLink(RGResource* input, RGResource* output);

    ///
    void compile();

    /// Executes the graph
    void execute();

private:
    std::vector<std::unique_ptr<RGPassNode>> passes;
    std::vector<std::unique_ptr<RGResourceNode>> resourceNodes;
    std::vector<std::unique_ptr<RGResource>> resources;
};

std::unique_ptr<RenderGraph> createDefaultRenderGraph();

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
