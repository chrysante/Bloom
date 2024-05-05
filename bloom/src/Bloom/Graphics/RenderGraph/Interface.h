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

#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"

namespace bloom::rg {

///
enum class InterfaceFlags {
    None,

    /// Indicates that an interface is not required. Only valid for sinks
    Optional,

    /// Indicates that the owning pass manually allocates the backing resource
    ManuallyAllocated // Not used yet
};

UTL_ENUM_OPERATORS(InterfaceFlags)

/// Type erasing wrapper around a member pointer to derived pass classes. This
/// exists so we don't have to parametrize `Interface` on the derived pass type
template <typename InterfaceView>
class InterfaceAccessor {
    template <typename PassType>
    using MemPtrType = InterfaceView(PassType::*);

public:
    InterfaceAccessor(std::nullptr_t = nullptr): fn(nullptr) {}

    template <typename PassType>
    InterfaceAccessor(MemPtrType<PassType> member): fn(impl<PassType>) {
        static_assert(sizeof(MemPtrType<PassType>) == sizeof(MemPtrType<Pass>));
        static_assert(alignof(MemPtrType<PassType>) ==
                      alignof(MemPtrType<Pass>));
        std::memcpy(&memptr, &member, sizeof member);
    }

    InterfaceView* operator()(Pass& pass) const {
        return fn ? fn(*this, pass) : nullptr;
    }

    explicit operator bool() const { return fn; }

private:
    template <typename PassType>
#if defined(__GNUC__)
    // See comment on PassImpl::PassImpl()
    __attribute__((no_sanitize("vptr")))
#endif
    static InterfaceView*
        impl(InterfaceAccessor const& This, Pass& pass) {
        using MemPtr = InterfaceView(PassType::*);
        auto member = *reinterpret_cast<MemPtr const*>(This.memptr);
        return &(static_cast<PassType&>(pass).*member);
    }

    InterfaceView* (*fn)(InterfaceAccessor const&, Pass&);
    alignas(MemPtrType<Pass>) char memptr[sizeof(MemPtrType<Pass>)];
};

/// Factory type that creates a concrete interface from frame parameters
///
/// This is used to dynamically describe interfaces based on frame parameters.
/// For example many passes expose a texture source with dimensions equal to the
/// screen size
template <typename Desc>
class InterfaceDescriptorFactory {
public:
    /// Creates an interface descriptor factory that is independent of the frame
    /// parameters
    InterfaceDescriptorFactory(Desc const& desc):
        impl([=](auto&) { return desc; }) {}

    /// Creates an interface descriptor factory from a callback
    template <std::invocable<FrameParameters const&> F>
    InterfaceDescriptorFactory(F&& f): impl(std::forward<F>(f)) {}

    /// Invokes the factory and returns the concrete interface descriptor
    Desc operator()(FrameParameters const& params) const {
        return impl(params);
    }

private:
    std::function<Desc(FrameParameters const&)> impl;
};

/// Abstract base class of concrete interfaces
class Interface {
public:
    std::string const& name() const { return _name; }

    InterfaceFlags flags() const { return _flags; }

    InterfaceType type() const { return _type; }

    void* accessor(Pass& pass) const;

protected:
    Interface(InterfaceType type, std::string name, InterfaceFlags flags):
        _type(type), _name(std::move(name)), _flags(flags) {}

private:
    friend InterfaceType dyncast_get_type(Interface const& i) {
        return i.type();
    }

    InterfaceType _type;
    std::string _name;
    InterfaceFlags _flags;
};

/// Buffer interface descriptor
struct BufferInterfaceDescriptor {
    /// The size of the requested buffer in bytes
    size_t size = 0;

    bool operator==(BufferInterfaceDescriptor const&) const = default;

    size_t hashValue() const { return std::hash<size_t>{}(size); }
};

class BufferInterface: public Interface {
public:
    using Resource = BufferResource;

    explicit BufferInterface(
        std::string name, InterfaceAccessor<BufferHandle> accessor,
        InterfaceDescriptorFactory<BufferInterfaceDescriptor> desc,
        InterfaceFlags flags = InterfaceFlags::None):
        Interface(InterfaceType::BufferInterface, std::move(name), flags),
        idf(std::move(desc)),
        acc(accessor) {}

    explicit BufferInterface(std::string name,
                             InterfaceAccessor<BufferHandle> accessor,
                             BufferInterfaceDescriptor desc,
                             InterfaceFlags flags = InterfaceFlags::None):
        BufferInterface(std::move(name), accessor,
                        InterfaceDescriptorFactory(desc), flags) {}

    InterfaceDescriptorFactory<BufferInterfaceDescriptor> IDF() const {
        return idf;
    }

private:
    friend class Interface;

    InterfaceDescriptorFactory<BufferInterfaceDescriptor> idf;
    InterfaceAccessor<BufferHandle> acc;
};

/// Texture interface descriptor
struct TextureInterfaceDescriptor {
    TextureType type = TextureType::Texture2D;
    PixelFormat pixelFormat = PixelFormat::RGBA8Unorm;
    vml::usize3 size{};
    size_t mipmapLevelCount = 1;
    size_t sampleCount = 1;
    size_t arrayLength = 1;
    TextureUsage usage = TextureUsage::ShaderRead | TextureUsage::RenderTarget;
    StorageMode storageMode = StorageMode::GPUOnly;

    bool operator==(TextureInterfaceDescriptor const&) const = default;

    size_t hashValue() const {
        return utl::hash_combine(type, pixelFormat, size, mipmapLevelCount,
                                 sampleCount, arrayLength, usage, storageMode);
    }
};

class TextureInterface: public Interface {
public:
    using Resource = TextureResource;

    explicit TextureInterface(
        std::string name, InterfaceAccessor<TextureHandle> accessor,
        InterfaceDescriptorFactory<TextureInterfaceDescriptor> desc,
        InterfaceFlags flags = InterfaceFlags::None):
        Interface(InterfaceType::TextureInterface, std::move(name), flags),
        idf(std::move(desc)),
        acc(accessor) {}

    explicit TextureInterface(std::string name,
                              InterfaceAccessor<TextureHandle> accessor,
                              TextureInterfaceDescriptor desc,
                              InterfaceFlags flags = InterfaceFlags::None):
        TextureInterface(std::move(name), accessor,
                         InterfaceDescriptorFactory(desc), flags) {}

    InterfaceDescriptorFactory<TextureInterfaceDescriptor> IDF() const {
        return idf;
    }

private:
    friend class Interface;

    InterfaceDescriptorFactory<TextureInterfaceDescriptor> idf;
    InterfaceAccessor<TextureHandle> acc;
};

class GeometryInterface: public Interface {
public:
    using Resource = GeometryResource;

    explicit GeometryInterface(std::string name,
                               InterfaceAccessor<GeometryResource*> accessor,
                               InterfaceFlags flags = InterfaceFlags::None):
        Interface(InterfaceType::GeometryInterface, std::move(name), flags),
        acc(accessor) {}

private:
    friend class Interface;

    InterfaceAccessor<GeometryResource*> acc;
};

class LightsInterface: public Interface {
public:
    using Resource = LightsResource;

    explicit LightsInterface(std::string name,
                             InterfaceAccessor<LightsResource*> accessor,
                             InterfaceFlags flags = InterfaceFlags::None):
        Interface(InterfaceType::LightsInterface, std::move(name), flags),
        acc(accessor) {}

private:
    friend class Interface;

    InterfaceAccessor<LightsResource*> acc;
};

union InterfaceUnion {
    template <typename T>
    static auto& getImpl(auto* This) {
        if (false)
            ;
#define RG_CONCRETE_RESOURCE_DEF(Name)                                         \
    else if constexpr (std::same_as<T, Name##Interface>) return This           \
        ->case##Name;
#include "Bloom/Graphics/RenderGraph/Resource.def"
        else
            ;
    }

public:
#define RG_CONCRETE_RESOURCE_DEF(Name)                                         \
    InterfaceUnion(Name##Interface const& impl):                               \
        case##Name(impl){} InterfaceUnion(Name##Interface && impl):            \
    case##Name(std::move(impl)){}
#include "Bloom/Graphics/RenderGraph/Resource.def"

    template <typename F>
    decltype(auto) visit(F&& f) {
        return bloom::visit(get(), std::forward<F>(f));
    }

    template <typename F>
    decltype(auto) visit(F&& f) const {
        return bloom::visit(get(), std::forward<F>(f));
    }

    InterfaceUnion(InterfaceUnion const& rhs) {
        rhs.visit([this]<typename I>(I const& rhs) {
            std::construct_at(&getImpl<I>(this), rhs);
        });
    }

    InterfaceUnion& operator=(InterfaceUnion const& rhs) {
        if (this == &rhs) {
            return *this;
        }
        std::destroy_at(this);
        std::construct_at(this, rhs);
        return *this;
    }

    InterfaceUnion(InterfaceUnion&& rhs) {
        rhs.visit([this]<typename I>(I& rhs) {
            std::construct_at(&getImpl<I>(this), std::move(rhs));
        });
    }

    InterfaceUnion& operator=(InterfaceUnion&& rhs) {
        if (this == &rhs) {
            return *this;
        }
        std::destroy_at(this);
        std::construct_at(this, std::move(rhs));
        return *this;
    }

    ~InterfaceUnion() {
        visit([]<typename I>(I const& This) { std::destroy_at(&This); });
    }

    Interface& get() {
        return const_cast<Interface&>(
            static_cast<InterfaceUnion const*>(this)->get());
    }

    Interface const& get() const {
        return reinterpret_cast<Interface const&>(*this);
    }

private:
#define RG_CONCRETE_RESOURCE_DEF(Name) Name##Interface case##Name;
#include "Bloom/Graphics/RenderGraph/Resource.def"
};

/// Base class of `PassSource` and `PassSink`
class PassTerminal {
public:
    virtual ~PassTerminal() = default;

    /// \Returns the interface of this terminal
    InterfaceUnion const& interface() const { return impl; }

    /// \Returns the implementation handle of this terminal
    void* handle() const { return _handle; }

    /// \Returns the name of this terminal. Shorthand for
    /// `interface().get().name()`
    std::string const& name() const { return interface().get().name(); }

    /// \Returns the name qualified with the name of the owning pass
    std::string qualifiedName() const;

    /// \Returns the pass the which this interface belongs
    Pass* pass() { return _pass; }

    /// \overload
    Pass const* pass() const { return _pass; }

    /// \Returns the backing resource or null if none exists
    Resource* resource() const { return _resource; }

    /// Assigns the backing resource
    void setResource(Resource* resource) { doSetResource(resource); }

    /// \Returns `interface().get().flags()`
    InterfaceFlags flags() const { return interface().get().flags(); }

    ///
    size_t indexInParent() const { return doIndexInParent(); }

protected:
    friend class RenderGraphInternalSink;
    friend class RenderGraphInternalSource;

    PassTerminal(InterfaceUnion interface, Pass* pass, void* handle):
        impl(std::move(interface)), _pass(pass), _handle(handle) {}

    /// Allow derived classes to customize this behaviour
    virtual void doSetResource(Resource* resource) { _resource = resource; }

    virtual size_t doIndexInParent() const = 0;

private:
    InterfaceUnion impl;
    Pass* _pass = nullptr;
    void* _handle;
    Resource* _resource = nullptr;
};

/// Resource input of a pass
class PassSink: public PassTerminal {
public:
    explicit PassSink(InterfaceUnion interface, Pass* pass, void* handle):
        PassTerminal(std::move(interface), pass, handle) {}

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
    size_t doIndexInParent() const final;

    PassSource* _source = nullptr;
};

/// Resource output of a pass
class PassSource: public PassTerminal {
public:
    explicit PassSource(InterfaceUnion interface, Pass* pass, void* handle):
        PassTerminal(std::move(interface), pass, handle) {}

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
    size_t doIndexInParent() const final;

    utl::small_vector<PassSink*, 3> _sinks;
};

/// The following classes model sinks and sources of render graphs:
///
/// ```
///                    ┌─ Render Graph ───────────────────┐
/// `RenderGraphSink` >>> `RenderGraphInternalSource`     │
///    An input into   │     Within the graph the         │
///    the graph       │     sink is represented by       │
///                    │     a source                     │
///                    │                                  │
///                    │       `RenderGraphInternalSink` >>> `RenderGraphSource`
///                    │           Within the graph the   │     An output of
///                    │           output is represented  │     the graph
///                    │           by a sink              │
///                    └──────────────────────────────────┘
/// ```
///
/// One thing to note is that `RenderGraphInternal{Sink/Source}::pass()` always
/// returns null, whereas `RenderGraph{Sink/Source}::pass()` returns the render
/// graph

/// Models a sink within a render graph that is a source of the render graph
class RenderGraphInternalSink: public PassSink {
public:
    explicit RenderGraphInternalSink(PassSource* owningSource,
                                     InterfaceUnion interface, void* handle):
        PassSink(std::move(interface), nullptr, handle),
        _owningSource(owningSource) {}

    /// \Returns the external source of this internal sink
    PassSource& owningSource() { return *_owningSource; }

    /// \overload
    PassSource const& owningSource() const { return *_owningSource; }

private:
    void doSetResource(Resource* resource) override {
        PassSink::doSetResource(resource);
        owningSource().PassSource::doSetResource(resource);
    }

    PassSource* _owningSource;
};

/// Models a source within a render graph that is a sink of the render graph
class RenderGraphInternalSource: public PassSource {
public:
    explicit RenderGraphInternalSource(PassSink* owningSink,
                                       InterfaceUnion interface, void* handle):
        PassSource(std::move(interface), nullptr, handle),
        _owningSink(owningSink) {}

    /// \Returns the external sink of this internal source
    PassSink& owningSink() { return *_owningSink; }

    /// \overload
    PassSink const& owningSink() const { return *_owningSink; }

private:
    void doSetResource(Resource* resource) override {
        PassSource::doSetResource(resource);
        owningSink().PassSink::doSetResource(resource);
    }

    PassSink* _owningSink;
};

/// Resource input of a render graph.
/// Models the sink into the graph and the corresponding source for passes
/// within the graph
class RenderGraphSink: public PassSink {
public:
    explicit RenderGraphSink(InterfaceUnion interface, Pass* pass,
                             void* handle):
        PassSink(interface, pass, handle), src(this, interface, handle) {}

    /// \Returns the source that makes this sink readable to passes within the
    /// render graph
    PassSource* internalSource() { return &src; }

    /// \overload
    PassSource const* internalSource() const { return &src; }

private:
    void doSetResource(Resource* resource) override {
        PassSink::doSetResource(resource);
        src.setResource(resource);
    }

    RenderGraphInternalSource src;
};

/// Resource output of a render graph.
/// Models the source of the graph and the corresponding sink for passes within
/// the graph
class RenderGraphSource: public PassSource {
public:
    explicit RenderGraphSource(InterfaceUnion interface, Pass* pass,
                               void* handle):
        PassSource(interface, pass, handle), snk(this, interface, handle) {}

    /// \Returns the sink that makes this source definable by passes within the
    /// render graph
    PassSink* internalSink() { return &snk; }

    /// \overload
    PassSink const* internalSink() const { return &snk; }

private:
    void doSetResource(Resource* resource) override {
        PassSource::doSetResource(resource);
        snk.setResource(resource);
    }

    RenderGraphInternalSink snk;
};

/// # Inline implementation

inline Pass* PassSink::getSourcePass() const {
    return source() ? source()->pass() : nullptr;
}

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
