#include "Bloom/Graphics/RenderGraph/Interface.h"

#include <range/v3/algorithm.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Graphics/RenderGraph/Pass.h"

using namespace bloom;

void* Interface::accessor(Pass& pass) const {
    return visit(*this,
                 [&](auto& interface) -> void* { return interface.acc(pass); });
}

static std::string getParentPassName(PassTerminal const& term) {
    if (term.pass()) {
        return term.pass()->name();
    }
    if (auto* s = dynamic_cast<RenderGraphInternalSink const*>(&term)) {
        return getParentPassName(s->owningSource());
    }
    if (auto* s = dynamic_cast<RenderGraphInternalSource const*>(&term)) {
        return getParentPassName(s->owningSink());
    }
    return "<unknown-pass>";
}

std::string PassTerminal::qualifiedName() const {
    auto qualName = utl::strcat(getParentPassName(*this), ".", name());
    auto itr = ranges::remove_if(qualName,
                                 [](char c) { return std::isspace((int)c); });
    qualName.erase(itr, qualName.end());
    return qualName;
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
