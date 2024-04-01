#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FWD_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FWD_H

#include <iosfwd>

#include "Bloom/Core/Dyncast.h"

namespace bloom {

class RenderContext;

} // namespace bloom

namespace bloom::rg {

class RenderGraph;
class Pass;

/// Forward declaration of resource nodes
#define RG_RESOURCE_DEF(Name, ...) class Name;
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"

/// Resource node type identifiers
enum class ResourceNodeType {
#define RG_RESOURCE_DEF(Name, ...) Name,
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"
    LAST = SceneLightsResource
};

///
std::ostream& operator<<(std::ostream& ostream, ResourceNodeType nodeType);

/// Forward declaration of resource node interfaces
#define RG_CONCRETE_RESOURCE_DEF(Name, ...) struct Name##Interface;
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"

using NoParent = void;

template <typename T>
struct IsInterface: std::false_type {};

#define RG_CONCRETE_RESOURCE_DEF(Name, ...)                                    \
    template <>                                                                \
    struct IsInterface<Name##Interface>: std::true_type {};
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"

template <typename T>
concept InterfaceType = IsInterface<T>::value;

template <typename T>
struct ToResourceType;

#define RG_CONCRETE_RESOURCE_DEF(Name, ...)                                    \
    template <>                                                                \
    struct ToResourceType<Name##Interface>:                                    \
        std::type_identity<Name##Resource> {};
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"

template <typename T>
struct ToInterfaceType;

#define RG_CONCRETE_RESOURCE_DEF(Name, ...)                                    \
    template <>                                                                \
    struct ToInterfaceType<Name##Resource>:                                    \
        std::type_identity<Name##Interface> {};
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"

} // namespace bloom::rg

#define RG_RESOURCE_DEF(Name, Parent, Corporeality)                            \
    UTL_DYNCAST_DEFINE(bloom::rg::Name, bloom::rg::ResourceNodeType::Name,     \
                       bloom::rg::Parent, Corporeality)
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FWD_H
