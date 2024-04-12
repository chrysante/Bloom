#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H

#include <memory>
#include <span>
#include <string>
#include <vector>

#include <utl/hashtable.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/ResourcePool.h"

namespace bloom::rg {

/// Data that is shared between nested render graphs
struct RenderGraphContext {
    ResourcePool resourcePool;
};

///
class RenderGraph: public Pass {
public:
    explicit RenderGraph(std::string name, RenderGraphContext& context,
                         HardwareDevice& device);

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
    PassSource* declareInput(std::string name, Interface interface) {
        inputs.push_back(std::make_unique<PassSource>(nullptr, std::move(name),
                                                      nullptr,
                                                      std::move(interface)));
        return inputs.back().get();
    }

    /// Links the source (output) \p source to the sink (input) \p sink
    void addLink(PassSource* source, PassSink* sink);

    /// Prepares the graph for execution
    bool compile();

    /// TODO: Document this
    void allocateResources(FrameParameters params);

    ///
    Pass* findPass(std::string_view name);

    ///
    PassSource* findInputSource(std::string_view name);

    /// \Returns the shared render graph context
    RenderGraphContext& context() const { return *ctx; }

    /// \Returns the associated device
    HardwareDevice& device() const { return *mDevice; }

private:
    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) final;

    RenderGraphContext* ctx;
    HardwareDevice* mDevice;
    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<std::unique_ptr<PassSource>> inputs;
    std::vector<Pass*> executionOrder;
    FrameParameters lastFrameParams;
};

/// Global render graph instance. This is a temporary function and will be
/// removed soon
RenderGraph* getGlobalRenderGraph(HardwareDevice* device);

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
