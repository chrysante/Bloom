#ifndef BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H

#include <span>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <utl/structure_of_arrays.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/Lights.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"
#include "Bloom/Graphics/Renderer2D.h"
#include "Bloom/Graphics/StaticMesh.h"

namespace bloom {

/// Base class of all resources
class Resource {
public:
    /// \Returns the most derived type of this resource
    ResourceNodeType type() const { return _type; }

    /// \Returns a view over all terminals that use this resource
    std::span<PassTerminal* const> users() const { return _users; }

    /// Adds \p terminal as a user of this resource
    void addUser(PassTerminal* terminal);

    ///
    void reallocate(HardwareDevice& device, FrameParameters const& params);

protected:
    template <typename Desc>
    using IDF = InterfaceDescriptorFactory<Desc>;

    Resource(ResourceNodeType type): _type(type) {}

    void assignUsers();

private:
    void doReallocate(HardwareDevice&, FrameParameters const&) {}

    friend ResourceNodeType dyncast_get_type(Resource const& resource) {
        return resource.type();
    }

    ResourceNodeType _type;
    utl::small_vector<PassTerminal*> _users;
};

/// Wrapper around a `BufferHandle`
class BufferResource: public Resource {
public:
    using ViewType = BufferHandle;

    ///
    explicit BufferResource(IDF<BufferInterfaceDescriptor> idf):
        Resource(ResourceNodeType::BufferResource), idf(std::move(idf)) {}

    ///
    BufferHandle const& handle() const { return _handle; }

    /// Assigns the underlying resource and updates all users
    void assign(BufferHandle buffer);

private:
    friend class Resource;

    void doReallocate(HardwareDevice& device, FrameParameters const& params);

    BufferHandle _handle;
    IDF<BufferInterfaceDescriptor> idf;
    BufferInterfaceDescriptor desc{};
};

/// Wrapper around a `TextureHandle`
class TextureResource: public Resource {
public:
    using ViewType = TextureHandle;

    ///
    explicit TextureResource(IDF<TextureInterfaceDescriptor> idf):
        Resource(ResourceNodeType::TextureResource), idf(std::move(idf)) {}

    ///
    TextureHandle const& handle() const { return _handle; }

    /// Assigns the underlying resource and updates all users
    void assign(TextureHandle texture);

private:
    friend class Resource;

    void doReallocate(HardwareDevice& device, FrameParameters const& params);

    TextureHandle _handle;
    IDF<TextureInterfaceDescriptor> idf;
    TextureInterfaceDescriptor desc{};
};

///
UTL_SOA_TYPE(Geometry, (Reference<StaticMeshRenderer>, mesh),
             (Reference<MaterialInstance>, materialInstance),
             (vml::float4x4, transform), (uint32_t, entityID));

/// Set of visible meshes in a scene. This is rebuild every frame
class GeometryResource: public Resource {
public:
    using ViewType = GeometryResource*;

    GeometryResource(): Resource(ResourceNodeType::GeometryResource) {}

    ///
    GeometryResource* handle() { return this; }

    /// Adds an object to the resource
    void add(Reference<StaticMeshRenderer> mesh,
             Reference<MaterialInstance> material,
             vml::float4x4 const& transform, uint32_t entityID);

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

    ///
    BufferHandle const& IDBuffer() const { return IDs; }

private:
    utl::structure_of_arrays<Geometry> objects;
    BufferHandle transforms;
    BufferHandle IDs;
};

/// Wrapper around `Renderer2DScene`
class Geometry2DResource: public Resource {
public:
    using ViewType = Geometry2DResource*;

    Geometry2DResource(): Resource(ResourceNodeType::Geometry2DResource) {}

    ///
    Geometry2DResource* handle() { return this; }

    Renderer2DScene scene;
};

/// Set of lights in a scene. This is rebuild every frame
class LightsResource: public Resource {
public:
    using ViewType = LightsResource*;

    LightsResource(): Resource(ResourceNodeType::LightsResource) {}

    ///
    LightsResource* handle() { return this; }

    void add(PointLight const& light);
    void add(SpotLight const& light);
    void add(DirectionalLight const& light);
    void add(SkyLight const& light);
    void setIrradianceCoefficients(std::span<vml::float3 const> coeffiencts);

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
    std::array<vml::float3, 9> irradianceCoefficients{};

    BufferHandle buffer;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_RESOURCE_H
