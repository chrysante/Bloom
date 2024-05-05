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

    /// Adds \p pass to this render graph.
    /// \Returns a pointer to the added pass.
    /// \pre this function must not be called after the graph is compiled.
    Pass* addPass(std::unique_ptr<Pass> pass);

    /// \overload
    template <std::derived_from<Pass> P>
    P* addPass(std::unique_ptr<P> pass) {
        auto* p = addPass(std::unique_ptr<Pass>(pass.release()));
        return static_cast<P*>(p);
    }

    /// Links the source (output) \p source to the sink (input) \p sink
    /// \pre this function must not be called after the graph is compiled.
    void addLink(PassSource* source, PassSink* sink);

    /// Checks that all relevant passes have all required sinks linked to a
    /// source and determines the execution order. Nested render graphs may be
    /// compiled separately but this function will recurse on any nested render
    /// graph that has not been compiled before.
    bool compile();

    /// \Returns `true` if this graph is compiled
    bool isCompiled() const { return !executionOrder.empty(); }

    /// Allocates the resource wrappers backing the passes in this graph. This
    /// must be called once by the top-level graph after compilation and before
    /// execution
    void allocateBackingResources();

    /// \Returns the pass with name \p name if it exists
    Pass* findPass(std::string_view name);

    ///
    PassSource* findInputSource(std::string_view name);

    /// \Returns the shared render graph context
    RenderGraphContext& context() const { return *ctx; }

    /// Declares a resource sink (input) to this graph
    RenderGraphSink* declareSink(InterfaceUnion interface) {
        return Pass::declareSink<RenderGraphSink>(std::move(interface),
                                                  nullptr);
    }

    /// Declares a resource source (output) to this graph
    RenderGraphSource* declareSource(InterfaceUnion interface) {
        return Pass::declareSource<RenderGraphSource>(std::move(interface),
                                                      nullptr);
    }

    ///
    std::unique_ptr<RenderGraph> clone(RenderGraphContext& context) const;

private:
    void doExecute(FrameParameters const& params, CommandQueue& queue) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    std::unique_ptr<Pass> doClone(RenderGraphContext& context) const override;

    RenderGraphContext* ctx;
    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<Pass*> executionOrder;
    FrameParameters lastFrameParams;
};

/// Temporary render graph manager until we figure out where to store them
struct RenderGraphManager {
    /// \Returns the global render graph instance
    static RenderGraph* get();

    /// Sets the global render graph
    static void set(std::unique_ptr<RenderGraph> rg);

    ///
    static RenderGraphContext& context();
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RENDERGRAPH_H
