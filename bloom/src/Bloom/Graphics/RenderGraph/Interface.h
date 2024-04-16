#ifndef BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H

#include <span>
#include <string>
#include <variant>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/common.hpp>
#include <utl/hash.hpp>
#include <utl/overload.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/SizeDescriptors.h"

namespace bloom::rg {

enum class InterfaceType { Buffer, Texture, Geometry, Lights };

template <InterfaceType Type>
using InterfaceViewType =
    std::tuple_element_t<(size_t)Type,
                         std::tuple<BufferHandle, TextureHandle,
                                    GeometryResource*, LightsResource*>>;

template <InterfaceType Type>
using ResourceType =
    std::tuple_element_t<(size_t)Type,
                         std::tuple<BufferResource, TextureResource,
                                    GeometryResource, LightsResource>>;

///
enum class InterfaceFlags {
    None,

    /// Indicates that an interface is not required. Only valid for sinks
    Optional
};

UTL_ENUM_OPERATORS(InterfaceFlags)

/// Stub interface descriptor 
template <InterfaceType Type>
struct InterfaceDesc {
    bool operator==(InterfaceDesc const&) const = default;

    size_t hashValue() const { return 0; }
};

using BufferInterfaceDesc = InterfaceDesc<InterfaceType::Buffer>;
using TextureInterfaceDesc = InterfaceDesc<InterfaceType::Texture>;

/// Buffer interface descriptor
template <>
struct InterfaceDesc<InterfaceType::Buffer> {
    /// The size of the requested buffer
    /// TODO: Make this a class that allows for special values
    size_t size;

    bool operator==(InterfaceDesc const&) const = default;

    size_t hashValue() const { return std::hash<size_t>{}(size); }
};

/// Texture interface descriptor
template <>
struct InterfaceDesc<InterfaceType::Texture> {
    ///
    TextureType type = TextureType::Texture2D;

    ///
    PixelFormat pixelFormat = PixelFormat::RGBA8Unorm;

    ///
    TextureSize size;

    ///
    size_t mipmapLevelCount = 1;

    ///
    size_t sampleCount = 1;

    ///
    size_t arrayLength = 1;

    bool operator==(InterfaceDesc const&) const = default;

    size_t hashValue() const {
        return utl::hash_combine(type, pixelFormat, size, mipmapLevelCount,
                                 sampleCount, arrayLength);
    }
};

/// Interfaces statically describe the sources and sinks of a pass
template <InterfaceType Type, typename PassType>
struct Interface {
    /// The name of this interface
    std::string name;

    /// The data member of the pass class that holds the handle of this
    /// interface
    InterfaceViewType<Type>(PassType::*impl);

    /// The interface descriptor. This is used for type checking and resource
    /// allocation
    InterfaceDesc<Type> desc = {};

    /// Additional interface flags
    InterfaceFlags flags = InterfaceFlags::None;
};

/// Base class of `PassSource` and `PassSink`
class PassInterfaceBase {
    using DescVariant = std::variant<InterfaceDesc<InterfaceType::Buffer>,
                                     InterfaceDesc<InterfaceType::Texture>,
                                     InterfaceDesc<InterfaceType::Geometry>,
                                     InterfaceDesc<InterfaceType::Lights>>;

public:
    virtual ~PassInterfaceBase() = default;

    /// \Returns the underlying `*Interface` variant
    DescVariant& get() { return _desc; }

    /// \overload
    DescVariant const& get() const { return _desc; }

    /// \Returns the underlying `Interface` of type \p T
    template <InterfaceType Type>
    auto& get() {
        return std::get<(size_t)Type>(get());
    }

    /// \overload
    template <InterfaceType Type>
    auto const& get() const {
        return std::get<(size_t)Type>(get());
    }

    ///
    template <InterfaceType Type>
    InterfaceViewType<Type>* handle() const {
        return static_cast<InterfaceViewType<Type>*>(_handle);
    }

    /// \Returns `std::visit(vis, get())`
    decltype(auto) visit(auto&& vis) const { return std::visit(vis, get()); }

    /// \Returns the name of this interface
    std::string const& name() const { return _name; }

    /// \Returns the pass the which this interface belongs
    Pass* pass() { return _pass; }

    /// \overload
    Pass const* pass() const { return _pass; }

    /// \Returns the backing resource or null if none exists
    Resource* resource() const { return _resource; }

    /// Assigns the backing resource
    void setResource(Resource* resource) { _resource = resource; }

    ///
    InterfaceFlags flags() const { return _flags; }

protected:
    template <InterfaceType Type>
    PassInterfaceBase(Pass* pass, std::string name,
                      InterfaceViewType<Type>* handle, InterfaceDesc<Type> desc,
                      InterfaceFlags flags):
        _name(std::move(name)),
        _handle(handle),
        _desc(std::move(desc)),
        _pass(pass),
        _flags(flags) {}

private:
    std::string _name;
    void* _handle;
    DescVariant _desc;
    Pass* _pass = nullptr;
    Resource* _resource = nullptr;
    InterfaceFlags _flags;
};

/// Resource input of a pass
class PassSink: public PassInterfaceBase {
public:
    template <InterfaceType Type>
    explicit PassSink(Pass* pass, std::string name,
                      InterfaceViewType<Type>* handle,
                      InterfaceDesc<Type> const& desc, InterfaceFlags flags):
        PassInterfaceBase(pass, std::move(name), handle, desc, flags) {}

    /// \Returns the source that is linked to this sink
    /// \Note `addSink` on \p source must be called manually
    PassSource* source() const { return _source; }

    /// \Returns the pass whose source is linked to this sink or null if no
    /// source is linked
    Pass* getSourcePass() const;

    ///
    void setSource(PassSource* source) { _source = source; }

    /// \Returns `true` if this sink is not required to be connected
    bool isOptional() const { return test(flags() & InterfaceFlags::Optional); }

private:
    PassSource* _source = nullptr;
};

/// Resource output of a pass
class PassSource: public PassInterfaceBase {
public:
    template <InterfaceType Type>
    explicit PassSource(Pass* pass, std::string name,
                        InterfaceViewType<Type>* handle,
                        InterfaceDesc<Type> const& desc, InterfaceFlags flags):
        PassInterfaceBase(pass, std::move(name), handle, desc, flags) {}

    /// \Returns the sinks that are linked to this source
    std::span<PassSink* const> sinks() const { return _sinks; }

    /// \Returns a view over the passes that use this source
    auto getSinkPasses() const {
        return sinks() | ranges::views::transform(
                             [](auto* sink) { return sink->pass(); });
    }

    /// Adds \p sink as a sink of this source
    /// \Note `setSource` on \p sink must be called manually
    void addSink(PassSink* sink);

private:
    utl::small_vector<PassSink*, 3> _sinks;
};

/// Resource input of a render graph.
/// Models the sink into the graph and the corresponding source for passes
/// within the graph
class RenderGraphSink: public PassSink {
public:
    template <InterfaceType Type>
    explicit RenderGraphSink(Pass* pass, std::string name,
                             InterfaceViewType<Type>* handle,
                             InterfaceDesc<Type> const& desc,
                             InterfaceFlags flags):
        PassSink(pass, std::move(name), handle, desc, flags),
        src(nullptr, name, handle, desc, flags) {
        internalSource()->addSink(this);
        setSource(internalSource());
    }

    /// \Returns the source that makes this sink readable to passes within the
    /// render graph
    PassSource* internalSource() { return &src; }

    /// \overload
    PassSource const* internalSource() const { return &src; }

private:
    PassSource src;
};

/// Resource output of a render graph.
/// Models the source of the graph and the corresponding sink for passes within
/// the graph
class RenderGraphSource: public PassSource {
public:
    template <InterfaceType Type>
    explicit RenderGraphSource(Pass* pass, std::string name,
                               InterfaceViewType<Type>* handle,
                               InterfaceDesc<Type> const& desc,
                               InterfaceFlags flags):
        PassSource(pass, name, handle, desc, flags),
        snk(nullptr, name, handle, desc, flags) {
        addSink(internalSink());
        internalSink()->setSource(this);
    }

    /// \Returns the sink that makes this source definable by passes within the
    /// render graph
    PassSink* internalSink() { return &snk; }

    /// \overload
    PassSink const* internalSink() const { return &snk; }

private:
    PassSink snk;
};

/// # Inline implementation

inline Pass* PassSink::getSourcePass() const {
    return source() ? source()->pass() : nullptr;
}

} // namespace bloom::rg

template <bloom::rg::InterfaceType Type>
struct std::hash<bloom::rg::InterfaceDesc<Type>> {
    size_t operator()(bloom::rg::InterfaceDesc<Type> const& desc) const {
        return desc.hashValue();
    }
};

#endif // BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
