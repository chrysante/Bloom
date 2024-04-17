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
class RenderGraphContext {
public:
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

    /// Declares a resource sink (input) to this graph
    template <InterfaceType Type>
    RenderGraphSink* declareSink(std::string name,
                                 InterfaceDesc<Type> const& desc) {
        return Pass::declareSink<RenderGraphSink>(std::move(name), nullptr,
                                                  desc);
    }

    /// Declares a resource source (output) to this graph
    template <InterfaceType Type>
    RenderGraphSource* declareSource(std::string name,
                                     InterfaceDesc<Type> const& interface) {
        return Pass::declareSource<RenderGraphSource>(std::move(name), nullptr,
                                                      interface);
    }

private:
    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) final;

    RenderGraphContext* ctx;
    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<Pass*> executionOrder;
    FrameParameters lastFrameParams;
};

/// Temporary render graph manager until we figure out where to store them
struct RenderGraphManager {
    /// \Returns the global render graph instance. This takes a device to
    static RenderGraph* get();

    /// Sets the global render graph
    static void set(std::unique_ptr<RenderGraph> rg);

    ///
    static RenderGraphContext& context();
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
