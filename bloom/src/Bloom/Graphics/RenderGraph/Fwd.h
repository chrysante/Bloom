#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FWD_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FWD_H

#include <iosfwd>

#include <vml/vml.hpp>

#include "Bloom/Core/Dyncast.h"

namespace bloom::rg {

class RenderGraphContext;
class RenderGraph;
class Pass;
class RenderPass;
class ComputePass;
class PassSink;
class PassSource;
struct PassRegistryEntry;
class PassRegistry;

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

/// Forward declaration of interface types
class Interface;
#define RG_CONCRETE_RESOURCE_DEF(Name) class Name##Interface;
#include "Bloom/Graphics/RenderGraph/Resource.def"

/// Interface type identifiers
enum class InterfaceType {
    Interface,
#define RG_CONCRETE_RESOURCE_DEF(Name) Name##Interface,
#include "Bloom/Graphics/RenderGraph/Resource.def"
    LAST = LightsInterface
};

using NoParent = void;

} // namespace bloom::rg

#define RG_RESOURCE_DEF(Name, Parent, Corporeality)                            \
    UTL_DYNCAST_DEFINE(bloom::rg::Name, bloom::rg::ResourceNodeType::Name,     \
                       bloom::rg::Parent, Corporeality)
#include "Bloom/Graphics/RenderGraph/Resource.def"

UTL_DYNCAST_DEFINE(bloom::rg::Interface, bloom::rg::InterfaceType::Interface,
                   void, Abstract)
#define RG_CONCRETE_RESOURCE_DEF(Name)                                         \
    UTL_DYNCAST_DEFINE(bloom::rg::Name##Interface,                             \
                       bloom::rg::InterfaceType::Name##Interface,              \
                       bloom::rg::Interface, Concrete)
#include "Bloom/Graphics/RenderGraph/Resource.def"

namespace bloom::rg {

///
struct FrameParameters {
    ///
    vml::int2 screenSize = 0;

    vml::int2 shadowMapSize = 0;

    size_t numShadowMaps = 0;

    bool operator==(FrameParameters const&) const = default;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FWD_H
