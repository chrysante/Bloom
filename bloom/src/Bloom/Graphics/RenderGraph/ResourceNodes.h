#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCENODES_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCENODES_H

#include <span>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <utl/structure_of_arrays.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/Lights.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/StaticMesh.h"

namespace bloom::rg {

enum class LinkType { Input, Output };

/// Base class of all resources
class Resource {
public:
    /// \Returns the most derived type of this resource
    ResourceNodeType type() const { return _type; }

    /// \Returns a view over all sinks and sources that use this resource
    std::span<PassInterfaceBase* const> users() const { return _users; }

    ///
    void addUser(PassInterfaceBase* user);

protected:
    Resource(ResourceNodeType type): _type(type) {}

    void assignUsers();

private:
    friend ResourceNodeType dyncast_get_type(Resource const& resource) {
        return resource.type();
    }

    ResourceNodeType _type;
    utl::small_vector<PassInterfaceBase*> _users;
};

///
class BufferResource: public Resource {
public:
    BufferResource(): Resource(ResourceNodeType::BufferResource) {}

    void assign(BufferHandle buffer);

    BufferHandle const& handle() { return _handle; }

private:
    BufferHandle _handle;
};

///
class TextureResource: public Resource {
public:
    TextureResource(): Resource(ResourceNodeType::TextureResource) {}

    void assign(TextureHandle texture);

    ///
    TextureHandle const& handle() { return _handle; }

private:
    TextureHandle _handle;
};

///
UTL_SOA_TYPE(Geometry, (Reference<StaticMeshRenderer>, mesh),
             (Reference<MaterialInstance>, materialInstance),
             (vml::float4x4, transform));

///
class GeometryResource: public Resource {
public:
    GeometryResource(): Resource(ResourceNodeType::GeometryResource) {}

    ///
    GeometryResource* handle() { return this; }

    ///
    void add(Reference<StaticMeshRenderer> mesh,
             Reference<MaterialInstance> material,
             vml::float4x4 const& transform);

    ///
    void clear();

    ///
    void prepare();

    ///
    utl::structure_of_arrays<Geometry> const& getObjects() const {
        return objects;
    }

    ///
    BufferHandle const& transformBuffer() const { return transforms; }

private:
    utl::structure_of_arrays<Geometry> objects;
    BufferHandle transforms;
};

///
class LightsResource: public Resource {
public:
    LightsResource(): Resource(ResourceNodeType::LightsResource) {}

    ///
    LightsResource* handle() { return this; }

    void add(PointLight const& light);
    void add(SpotLight const& light);
    void add(DirectionalLight const& light);
    void add(SkyLight const& light);

    ///
    void clear();

    ///
    void prepare();

    ///
    BufferHandle const& buffer() const { return _buffer; }

private:
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<DirectionalLight> dirLights;
    std::vector<SkyLight> skyLights;

    BufferHandle _buffer;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCENODES_H
