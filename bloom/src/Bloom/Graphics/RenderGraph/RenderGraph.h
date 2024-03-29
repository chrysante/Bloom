#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H

#include <vector>

#include "Bloom/Graphics/RenderGraph/Node.h"

namespace bloom {

///
class RenderGraph {
public:
    ///
    RGPassNode* addPass(std::unique_ptr<RGPassNode> pass) {
        auto* result = pass.get();
        passes.push_back(std::move(pass));
        return result;
    }

    ///
    RGResourceNode* addResource(std::unique_ptr<RGResourceNode> resource) {
        auto* result = resource.get();
        resources.push_back(std::move(resource));
        return result;
    }

private:
    std::vector<std::unique_ptr<RGPassNode>> passes;
    std::vector<std::unique_ptr<RGResourceNode>> resources;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
