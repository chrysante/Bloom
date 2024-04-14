#ifndef BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_PASS_H

#include <concepts>
#include <string>
#include <vector>

#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"

namespace bloom::rg {

/// Base class for all render graph pass nodes.
class Pass {
public:
    explicit Pass(std::string name): _name(std::move(name)) {}

    Pass(Pass const&) = delete;

    virtual ~Pass();

    /// \Returns the debug name of this pass
    std::string const& name() const { return _name; };

    /// \Returns the parent render graph if this pass is part of a render graph
    RenderGraph* parent() const { return _parent; }

    /// \Returns the resource sink (input) with name \p name
    PassSink* findSink(std::string_view name);

    /// \Returns the resource source (output) with name \p name
    PassSource* findSource(std::string_view name);

    /// \Returns the sink at index \p index
    PassSink* sink(size_t index) {
        return const_cast<PassSink*>(
            static_cast<Pass const*>(this)->sink(index));
    }

    /// \overload
    PassSink const* sink(size_t index) const {
        BL_EXPECT(index < _sinks.size());
        return _sinks[index].get();
    }

    /// \Returns a view over the resource sinks of this pass
    auto sinks() { return _sinks | ToAddress<>; }

    /// \overload
    auto sinks() const { return _sinks | ToAddress<PassSink const>; }

    /// \Returns a view over the sinks that are connected to a resource
    auto activeSinks() {
        return sinks() | ranges::views::filter(
                             [](auto* s) -> bool { return s->source(); });
    }

    /// \overload
    auto activeSinks() const {
        return sinks() | ranges::views::filter(
                             [](auto* s) -> bool { return s->source(); });
    }

    /// \Returns the source at index \p index
    PassSource* source(size_t index) {
        return const_cast<PassSource*>(
            static_cast<Pass const*>(this)->source(index));
    }

    /// \overload
    PassSource const* source(size_t index) const {
        BL_EXPECT(index < _sources.size());
        return _sources[index].get();
    }

    /// \Returns a view over the resource sources of this pass
    auto sources() { return _sources | ToAddress<>; }

    /// \overload
    auto sources() const { return _sources | ToAddress<PassSource const>; }

    /// Sink passes are passes that are executed regardless of other passes
    /// depending on them \Returns `true` if this pass is a sink pass
    bool isSink() const { return _isSink; }

    /// Mark this pass as a sink pass
    void markSink() { _isSink = true; }

    /// Called by the render graph on resource reallocation
    void allocateResources(FrameParameters const& frameParams,
                           HardwareDevice& device) {
        doAllocateResources(frameParams, device);
    }

    /// Executes this pass
    void execute(FrameParameters const& params, CommandQueue& queue);

protected:
    /// Declares a resource sink (input) to this pass
    template <std::derived_from<PassSink> S = PassSink, InterfaceType Interface>
    S* declareSink(std::string name, Interface::ViewType* handle,
                   Interface interface) {
        _sinks.push_back(std::make_unique<S>(this, std::move(name), handle,
                                             std::move(interface)));
        return static_cast<S*>(_sinks.back().get());
    }

    /// Declares a resource source (output) to this pass
    template <std::derived_from<PassSource> S = PassSource,
              InterfaceType Interface>
    S* declareSource(std::string name, Interface::ViewType* handle,
                     Interface const& interface) {
        _sources.push_back(std::make_unique<S>(this, std::move(name), handle,
                                               std::move(interface)));
        return static_cast<S*>(_sources.back().get());
    }

private:
    friend class RenderGraph; // To set parent pointer

    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) = 0;
    virtual void doAllocateResources(FrameParameters const&, HardwareDevice&) {}

    bool _isSink = false;
    RenderGraph* _parent = nullptr;
    std::string _name;
    std::vector<std::unique_ptr<PassSink>> _sinks;
    std::vector<std::unique_ptr<PassSource>> _sources;
};

/// Base class for all passes that use the GPUs render pipeline
class RenderPass: public Pass {
public:
    using Pass::Pass;

protected:
    explicit RenderPass(std::string name): Pass(std::move(name)) {}

private:
    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) final;
    virtual void doExecute(FrameParameters const& params,
                           RenderContext& ctx) = 0;
};

/// Base class for all passes that use the GPUs compute pipeline
class ComputePass: public Pass {
public:
    using Pass::Pass;

protected:
    explicit ComputePass(std::string name): Pass(std::move(name)) {}

private:
    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) final;
    virtual void doExecute(FrameParameters const& params,
                           ComputeContext& ctx) = 0;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
