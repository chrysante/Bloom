#include "Bloom/Graphics/RenderGraph/Interface.h"

#include <range/v3/algorithm.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Graphics/RenderGraph/Pass.h"

using namespace bloom;
using namespace rg;

void* Interface::accessor(Pass& pass) const {
    return visit(*this,
                 [&](auto& interface) -> void* { return interface.acc(pass); });
}

std::string PassTerminal::qualifiedName() const {
    return utl::strcat(pass() ? pass()->name() : "?", ".", name());
}

size_t PassSink::doIndexInParent() const {
    BL_EXPECT(pass());
    auto sinks = pass()->sinks();
    return ranges::find(sinks, this) - sinks.begin();
}

size_t PassSource::doIndexInParent() const {
    BL_EXPECT(pass());
    auto sources = pass()->sources();
    return ranges::find(sources, this) - sources.begin();
}

void PassSource::addSink(PassSink* sink) {
    if (!ranges::contains(_sinks, sink)) {
        _sinks.push_back(sink);
    }
}
