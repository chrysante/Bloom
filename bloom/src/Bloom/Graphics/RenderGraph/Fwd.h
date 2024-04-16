#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FWD_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FWD_H

#include <iosfwd>

#include <vml/vml.hpp>

#include "Bloom/Core/Dyncast.h"

namespace bloom::rg {

class RenderGraph;
class Pass;
class RenderPass;
class ComputePass;
class PassSink;
class PassSource;

/// Forward declaration of resource nodes
#define RG_RESOURCE_DEF(Name, ...) class Name;
#include "Bloom/Graphics/RenderGraph/Resource.def"

/// Resource node type identifiers
enum class ResourceNodeType {
#define RG_RESOURCE_DEF(Name, ...) Name,
#include "Bloom/Graphics/RenderGraph/Resource.def"
    LAST = LightsResource
};

///
std::ostream& operator<<(std::ostream& ostream, ResourceNodeType nodeType);

using NoParent = void;

} // namespace bloom::rg

#define RG_RESOURCE_DEF(Name, Parent, Corporeality)                            \
    UTL_DYNCAST_DEFINE(bloom::rg::Name, bloom::rg::ResourceNodeType::Name,     \
                       bloom::rg::Parent, Corporeality)
#include "Bloom/Graphics/RenderGraph/Resource.def"

namespace bloom::rg {

///
struct FrameParameters {
    ///
    vml::int2 screenSize = 0;

    bool operator==(FrameParameters const&) const = default;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FWD_H
