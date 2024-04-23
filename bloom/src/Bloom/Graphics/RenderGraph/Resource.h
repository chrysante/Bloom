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
    static constexpr InterfaceType Type = InterfaceType::Buffer;

    using Desc = InterfaceDesc<Type>;

    ///
    explicit BufferResource(Desc desc):
        Resource(ResourceNodeType::BufferResource), _desc(desc) {}

    ///
    void assign(BufferHandle buffer);

    ///
    BufferHandle const& handle() { return _handle; }

    ///
    Desc const& desc() const { return _desc; }

private:
    BufferHandle _handle;
    Desc _desc;
};

/// Wrapper around a `TextureHandle`
class TextureResource: public Resource {
public:
    static constexpr InterfaceType Type = InterfaceType::Texture;

    using Desc = InterfaceDesc<Type>;

    ///
    explicit TextureResource(Desc desc):
        Resource(ResourceNodeType::TextureResource), _desc(desc) {}

    ///
    void assign(TextureHandle texture);

    ///
    TextureHandle const& handle() { return _handle; }

    ///
    Desc const& desc() const { return _desc; }

private:
    TextureHandle _handle;
    Desc _desc;
};

///
UTL_SOA_TYPE(Geometry, (Reference<StaticMeshRenderer>, mesh),
             (Reference<MaterialInstance>, materialInstance),
             (vml::float4x4, transform));

/// Set of visible meshes in a scene. This is rebuild every frame
class GeometryResource: public Resource {
public:
    static constexpr InterfaceType Type = InterfaceType::Geometry;

    using Desc = InterfaceDesc<Type>;

    explicit GeometryResource(Desc):
        Resource(ResourceNodeType::GeometryResource) {}

    ///
    GeometryResource* handle() { return this; }

    /// Adds an object to the resource
    void add(Reference<StaticMeshRenderer> mesh,
             Reference<MaterialInstance> material,
             vml::float4x4 const& transform);

    /// Must be called when starting a new frame
    void clear();

    /// Uploads the transform buffer
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
    static constexpr InterfaceType Type = InterfaceType::Lights;

    using Desc = InterfaceDesc<Type>;

    explicit LightsResource(Desc): Resource(ResourceNodeType::LightsResource) {}

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
    BufferHandle const& lightBuffer() const { return buffer; }

private:
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<DirectionalLight> dirLights;
    std::vector<SkyLight> skyLights;

    BufferHandle buffer;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H
