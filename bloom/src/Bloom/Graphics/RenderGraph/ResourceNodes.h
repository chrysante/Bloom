#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCENODES_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCENODES_H

#include <range/v3/algorithm.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"

namespace bloom::rg {

enum class LinkType { Input, Output };

/// Base class of all resource nodes.
/// Resource nodes act as user inputs to the graph and as pass outputs.
/// Resource nodes don't actually own resources. Instead they hold handles to
/// resources that are owned by the render graph. It is also the responsibility
/// of the render graph to populate the resource nodes.
class Resource {
public:
    /// \Returns the runtime type of this node
    ResourceNodeType type() const { return _type; }

    std::string const& name() const;

    /// The pass owning this resource. This may be null
    Pass* parent() const { return _parent; }

    ///
    std::span<Resource* const> successors() const { return succs; }

    ///
    Resource* predecessor() const { return pred; }

    ///
    LinkType linkType() const { return _linkType; }

    ///
    bool isInput() const { return linkType() == LinkType::Input; }

    ///
    bool isOutput() const { return linkType() == LinkType::Output; }

protected:
    Resource(ResourceNodeType type, LinkType linkType, Pass* parent):
        _type(type), _linkType(linkType), _parent(parent) {}

private:
    friend ResourceNodeType dyncast_get_type(Resource const& resource) {
        return resource.type();
    }

    friend class RenderGraph;

    void addSuccessor(Resource* succ) {
        if (!ranges::contains(succs, succ)) {
            succs.push_back(succ);
        }
    }

    void setPredecessor(Resource* pred) { this->pred = pred; }

    ResourceNodeType _type;
    LinkType _linkType;
    Pass* _parent = nullptr;
    utl::small_vector<Resource*> succs;
    Resource* pred = nullptr;
};

///
template <typename Derived, typename InterfaceType>
class ResourceMixin {
    using ViewType = InterfaceType::ViewType;

public:
    ///
    InterfaceType const& interface() const { return _interface; }

    ///
    void set(ViewType view) {
        BL_ASSERT(_interface.handle, "Handle not set");
        *_interface.handle = view;
    }

protected:
    ResourceMixin(InterfaceType interface): _interface(std::move(interface)) {}

private:
    InterfaceType _interface;
};

///
class BufferResource:
    public Resource,
    public ResourceMixin<BufferResource, BufferInterface> {
public:
    explicit BufferResource(BufferInterface interface, LinkType linkType,
                            Pass* parent):
        Resource(ResourceNodeType::BufferResource, linkType, parent),
        ResourceMixin(std::move(interface)) {}
};

///
class TextureResource:
    public Resource,
    public ResourceMixin<TextureResource, TextureInterface> {
public:
    explicit TextureResource(TextureInterface interface, LinkType linkType,
                             Pass* parent):
        Resource(ResourceNodeType::TextureResource, linkType, parent),
        ResourceMixin(std::move(interface)) {}
};

///
class SceneGeometryResource:
    public Resource,
    public ResourceMixin<SceneGeometryResource, SceneGeometryInterface> {
public:
    explicit SceneGeometryResource(SceneGeometryInterface interface,
                                   LinkType linkType, Pass* parent):
        Resource(ResourceNodeType::SceneGeometryResource, linkType, parent),
        ResourceMixin(std::move(interface)) {}
};

///
class SceneLightsResource:
    public Resource,
    public ResourceMixin<SceneLightsResource, SceneLightsInterface> {
public:
    explicit SceneLightsResource(SceneLightsInterface interface,
                                 LinkType linkType, Pass* parent):
        Resource(ResourceNodeType::SceneLightsResource, linkType, parent),
        ResourceMixin(std::move(interface)) {}
};

inline std::string const& Resource::name() const {
    return visit(*this, [](auto& resource) -> auto& {
        return resource.interface().name;
    });
}

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCENODES_H
