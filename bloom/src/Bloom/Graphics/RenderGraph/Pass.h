#ifndef BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_PASS_H

#include <string>
#include <vector>

#include <range/v3/view.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/ResourceNodes.h"

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

    ///
    Resource* findInput(std::string_view name);

    ///
    Resource* findOutput(std::string_view name);

    ///
    auto inputs() { return _inputs | ToAddress<>; }

    /// \overload
    auto inputs() const { return _inputs | ToAddress<Resource const>; }

    ///
    auto outputs() { return _outputs | ToAddress<>; }

    /// \overload
    auto outputs() const { return _outputs | ToAddress<Resource const>; }

    ///
    void markSink() { _isSink = true; }

    bool isSink() const { return _isSink; }

protected:
    Pass(std::string name): _name(std::move(name)) {}

    ///
    template <InterfaceType Interface>
    void declareInput(Interface const& interface) {
        _inputs.push_back(
            allocate<typename ToResourceType<Interface>::type>(interface,
                                                               LinkType::Input,
                                                               this));
    }

    ///
    template <InterfaceType Interface>
    void declareOutput(Interface const& interface) {
        _outputs.push_back(
            allocate<typename ToResourceType<Interface>::type>(interface,
                                                               LinkType::Output,
                                                               this));
    }

private:
    virtual void doExecute(RenderContext& ctx) = 0;

    std::string _name;
    std::vector<DynUniquePtr<Resource>> _inputs;
    std::vector<DynUniquePtr<Resource>> _outputs;
    bool _isSink = false;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_PASS_H
