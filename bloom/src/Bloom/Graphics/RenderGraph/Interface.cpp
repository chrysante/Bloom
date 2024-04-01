#include "Bloom/Graphics/RenderGraph/Interface.h"

#include <range/v3/algorithm.hpp>

using namespace bloom;
using namespace rg;

void PassSource::addSink(PassSink* sink) {
    if (!ranges::contains(_sinks, sink)) {
        _sinks.push_back(sink);
    }
}
