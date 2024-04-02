#ifndef BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_PASS_H

#include <string>
#include <vector>

#include <range/v3/view.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"

namespace bloom::rg {

/// Base class for all render graph pass nodes
class Pass {
public:
    Pass(Pass const&) = delete;

    virtual ~Pass();

    /// \Returns the debug name of this pass
    std::string const& name() const { return _name; };

    /// Renders this pass
    void execute(RenderContext& ctx) { doExecute(ctx); }

    /// \Returns the resource sink (input) with name \p name
    PassSink* findSink(std::string_view name);

    /// \Returns the resource source (output) with name \p name
    PassSource* findSource(std::string_view name);

    /// \Returns a view over the resource sinks of this pass
    auto sinks() { return _sinks | ToAddress<>; }

    /// \overload
    auto sinks() const { return _sinks | ToAddress<PassSink const>; }

    /// \Returns a view over the resource sources of this pass
    auto sources() { return _sources | ToAddress<>; }

    /// \overload
    auto sources() const { return _sources | ToAddress<PassSource const>; }

    /// Sink passes are passes that are executed regardless of other passes
    /// depending on them \Returns `true` if this pass is a sink pass
    bool isSink() const { return _isSink; }

    /// Mark this pass as a sink pass
    void markSink() { _isSink = true; }

protected:
    explicit Pass(std::string name): _name(std::move(name)) {}

    /// Declares a resource sink (input) to this pass
    template <InterfaceType Interface>
    void declareSink(std::string name, Interface::ViewType* handle,
                     Interface interface) {
        _sinks.push_back(std::make_unique<PassSink>(this, std::move(name),
                                                    handle,
                                                    std::move(interface)));
    }

    /// Declares a resource source (output)) to this pass
    template <InterfaceType Interface>
    void declareSource(std::string name, Interface::ViewType* handle,
                       Interface const& interface) {
        _sources.push_back(std::make_unique<PassSource>(this, std::move(name),
                                                        handle,
                                                        std::move(interface)));
    }

private:
    virtual void doExecute(RenderContext& ctx) = 0;

    std::string _name;
    std::vector<std::unique_ptr<PassSink>> _sinks;
    std::vector<std::unique_ptr<PassSource>> _sources;
    bool _isSink = false;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
