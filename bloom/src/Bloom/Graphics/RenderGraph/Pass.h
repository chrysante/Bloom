#ifndef BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_PASS_H

#include <array>
#include <concepts>
#include <string>
#include <vector>

#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"

namespace bloom {

template <typename ConcretePass, typename Base = ConcretePass::BasePass>
class PassImpl;

/// Base class for all render graph pass nodes.
class Pass {
    template <typename, typename>
    friend class PassImpl;

public:
    Pass(Pass const&) = delete;

    virtual ~Pass();

    /// \Returns the debug name of this pass
    std::string const& name() const { return _name; };

    /// \Returns the parent render graph if this pass is part of a render graph
    RenderGraph* parent() const { return _parent; }

    /// \Returns the associated device.
    /// \Pre This pass must be a render graph or part of a render graph
    HardwareDevice& device() const { return *_device; }

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

    /// Called by the render graph on resource reallocation
    void allocateResources(FrameParameters const& frameParams,
                           HardwareDevice& device) {
        doAllocateResources(frameParams, device);
    }

    /// Executes this pass
    void execute(FrameParameters const& params, CommandQueue& queue);

protected:
    explicit Pass(std::string name, HardwareDevice* device = nullptr):
        _device(device), _name(std::move(name)) {}

    /// Declares a resource sink (input) to this pass
    template <std::derived_from<PassSink> S = PassSink>
    S* declareSink(InterfaceUnion interface, void* handle) {
        _sinks.push_back(
            std::make_unique<S>(std::move(interface), this, handle));
        return static_cast<S*>(_sinks.back().get());
    }

    /// Declares a resource source (output) to this pass
    template <std::derived_from<PassSource> S = PassSource>
    S* declareSource(InterfaceUnion interface, void* handle) {
        _sources.push_back(
            std::make_unique<S>(std::move(interface), this, handle));
        return static_cast<S*>(_sources.back().get());
    }

private:
    friend class RenderGraph; // To set parent and device

    std::unique_ptr<Pass> clone() const { return doClone(); }

    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) = 0;

    virtual void doAllocateResources(FrameParameters const&, HardwareDevice&) {}

    virtual std::unique_ptr<Pass> doClone() const = 0;

    RenderGraph* _parent = nullptr;
    HardwareDevice* _device = nullptr;
    std::string _name;
    std::vector<std::unique_ptr<PassSink>> _sinks;
    std::vector<std::unique_ptr<PassSource>> _sources;
};

/// Base class for all passes that use the GPUs render pipeline
class RenderPass: public Pass {
protected:
    using Pass::Pass;
    explicit RenderPass(std::string name): Pass(std::move(name)) {}

private:
    void doExecute(FrameParameters const& params, CommandQueue& queue) final;
    virtual void doExecute(FrameParameters const& params,
                           RenderContext& ctx) = 0;
};

/// Base class for all passes that use the GPUs compute pipeline
class ComputePass: public Pass {
protected:
    using Pass::Pass;

    explicit ComputePass(std::string name): Pass(std::move(name)) {}

private:
    virtual void doExecute(FrameParameters const& params,
                           CommandQueue& queue) final;
    virtual void doExecute(FrameParameters const& params,
                           ComputeContext& ctx) = 0;
};

/// Mixin base class for special purpose passes.
/// This class exists to simplify defining pass interfaces in code
template <typename PassType, std::derived_from<Pass> Base>
class PassImpl<PassType, Base>: public Base {
    enum class CloneTag;

public:
    using BasePass = Base;

    /// To be defined by the pass author
    static std::span<InterfaceUnion const> Sinks();

    /// To be defined by the pass author
    static std::span<InterfaceUnion const> Sources();

    template <typename... Args>
    explicit PassImpl(CloneTag, Args&&... args):
        PassImpl(std::forward<Args>(args)...) {}

protected:
    /// Declares the sinks and sources defined in `Sinks` and `Sources` to the
    /// pass base class
    template <typename... Args>
#if defined(__GNUC__)
    // UBSan flags an issue with the static_cast<PassType*>(this). This seems to
    // be a false positive of UBSan because casting to a derived class under
    // construction should be well defined behaviour
    __attribute__((no_sanitize("vptr")))
#endif
    PassImpl(Args&&... args):
        Base(std::forward<Args>(args)...) {
        static_assert(
            std::derived_from<PassType, Pass>,
            "The pass must publicly derive from the `Pass` base class");
        auto* pass = static_cast<PassType*>(this);
        for (auto const& interface: PassType::Sinks()) {
            pass->declareSink(interface, interface.base().accessor(*pass));
        }
        for (auto const& interface: PassType::Sources()) {
            pass->declareSource(interface, interface.base().accessor(*pass));
        }
    }

private:
    std::unique_ptr<Pass> doClone() const override {
        return std::make_unique<PassType>(CloneTag{}, this->name());
    }
};

#define BEGIN_INTERFACE_DECL_IMPL(Name, PassType)                              \
    template <>                                                                \
    ::std::span<InterfaceUnion const> bloom::PassImpl<PassType>::Name() {      \
        static std::vector<InterfaceUnion> list

#define BEGIN_SINK_DECL(PassType) BEGIN_INTERFACE_DECL_IMPL(Sinks, PassType)

#define BEGIN_SOURCE_DECL(PassType) BEGIN_INTERFACE_DECL_IMPL(Sources, PassType)

#define END_DECL()                                                             \
    ;                                                                          \
    return list;                                                               \
    }

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
