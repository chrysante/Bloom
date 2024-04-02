#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H

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

namespace bloom {
class HardwareDevice;
} // namespace bloom

namespace bloom::rg {

/// Base class of all resources
class Resource {
public:
    /// \Returns the most derived type of this resource
    ResourceNodeType type() const { return _type; }

    /// \Returns a view over all sinks and sources that use this resource
    std::span<PassInterfaceBase* const> users() const { return _users; }

    /// Adds \p interface as a user of this resource
    void addUser(PassInterfaceBase* interface);

    ///
    void assignUsers();

protected:
    Resource(ResourceNodeType type): _type(type) {}

private:
    friend ResourceNodeType dyncast_get_type(Resource const& resource) {
        return resource.type();
    }

    ResourceNodeType _type;
    utl::small_vector<PassInterfaceBase*> _users;
};

/// Wrapper around a `BufferHandle`
class BufferResource: public Resource {
public:
    ///
    explicit BufferResource(BufferInterface interface):
        Resource(ResourceNodeType::BufferResource), _interface(interface) {}

    ///
    void assign(BufferHandle buffer);

    ///
    BufferHandle const& handle() { return _handle; }

    ///
    BufferInterface const& interface() const { return _interface; }

private:
    BufferHandle _handle;
    BufferInterface _interface;
};

/// Wrapper around a `TextureHandle`
class TextureResource: public Resource {
public:
    ///
    explicit TextureResource(TextureInterface interface):
        Resource(ResourceNodeType::TextureResource), _interface(interface) {}

    ///
    void assign(TextureHandle texture);

    ///
    TextureHandle const& handle() { return _handle; }

    ///
    TextureInterface const& interface() const { return _interface; }

private:
    TextureHandle _handle;
    TextureInterface _interface;
};

///
UTL_SOA_TYPE(Geometry, (Reference<StaticMeshRenderer>, mesh),
             (Reference<MaterialInstance>, materialInstance),
             (vml::float4x4, transform));

/// Set of meshes in a scene. This is rebuild every frame
class GeometryResource: public Resource {
public:
    explicit GeometryResource(GeometryInterface):
        Resource(ResourceNodeType::GeometryResource) {}

    ///
    GeometryResource* handle() { return this; }

    ///
    void add(Reference<StaticMeshRenderer> mesh,
             Reference<MaterialInstance> material,
             vml::float4x4 const& transform);

    ///
    void clear();

    ///
    void prepare(HardwareDevice& device);

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

/// Set of lights in a scene. This is rebuild every frame
class LightsResource: public Resource {
public:
    explicit LightsResource(LightsInterface):
        Resource(ResourceNodeType::LightsResource) {}

    ///
    LightsResource* handle() { return this; }

    void add(PointLight const& light);
    void add(SpotLight const& light);
    void add(DirectionalLight const& light);
    void add(SkyLight const& light);

    ///
    void clear();

    ///
    void prepare(HardwareDevice& device);

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

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H
