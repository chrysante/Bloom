#ifndef BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H

#include <span>
#include <string>
#include <variant>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hash.hpp>
#include <utl/overload.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"

namespace bloom::rg {

///
struct BufferInterface {
    using ViewType = BufferHandle;

    ///
    size_t size;

    struct Compare {
        bool operator()(BufferInterface const& lhs,
                        BufferInterface const& rhs) const {
            return lhs.size == rhs.size;
        }
    };

    struct Hash {
        size_t operator()(BufferInterface const& interface) const {
            return std::hash<size_t>{}(interface.size);
        }
    };
};

/// Size descriptor that supports absolute and relative values
struct TextureSize {
    struct Absolute {
        size_t size;
        bool operator==(Absolute const&) const = default;
    };
    struct RelativeToScreen {
        double factor = 1.0;
        bool operator==(RelativeToScreen const&) const = default;
    };

    using Element = std::variant<Absolute, RelativeToScreen>;

    vml::vector<Element, 3> value;

    /// Size descriptor that describes the screen size
    static TextureSize const ScreenSize;

    ///
    static TextureSize Relative2D(vml::double2 factor) {
        return { { RelativeToScreen{ factor.x }, RelativeToScreen{ factor.y },
                   Absolute{ 1 } } };
    }

    bool operator==(TextureSize const&) const = default;

    ///
    bool isAbsolute() const {
        return ranges::all_of(value, [](auto& elem) {
            return std::holds_alternative<Absolute>(elem);
        });
    }

    vml::usize3 compute(vml::usize2 screenSize) const {
        return vml::map(value, vml::usize3(screenSize, 1),
                        [&](auto elem, size_t screen) {
            // clang-format off
            return std::visit(utl::overload{
                [&](Absolute a) { return a.size; },
                [&](RelativeToScreen r) -> size_t {
                    return r.factor * screen;
                },
            }, elem); // clang-format on
        });
    }

    struct Hash {
        size_t operator()(TextureSize const& size) const {
            size_t seed = 0;
            for (auto& elem: size.value) {
                // clang-format off
                utl::hash_combine_seed(seed, std::visit(utl::overload{
                    [](Absolute a) { return std::hash<size_t>{}(a.size); },
                    [](RelativeToScreen r) {
                        return std::hash<double>{}(r.factor);
                    },
                }, elem)); // clang-format on
            }
            return seed;
        }
    };
};

constexpr TextureSize TextureSize::ScreenSize = { {
    RelativeToScreen{ 1.0 },
    RelativeToScreen{ 1.0 },
    Absolute{ 1 },
} };

///
struct TextureInterface {
    using ViewType = TextureHandle;

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

    struct Compare {
        bool operator()(TextureInterface const& lhs,
                        TextureInterface const& rhs) const {
            return lhs.type == rhs.type && lhs.pixelFormat == rhs.pixelFormat &&
                   lhs.size == rhs.size &&
                   lhs.mipmapLevelCount == rhs.mipmapLevelCount &&
                   lhs.sampleCount == rhs.sampleCount &&
                   lhs.arrayLength == rhs.arrayLength;
        }
    };

    struct Hash {
        size_t operator()(TextureInterface const& interface) const {
            return utl::hash_combine(interface.type, interface.pixelFormat,
                                     TextureSize::Hash{}(interface.size),
                                     interface.mipmapLevelCount,
                                     interface.sampleCount,
                                     interface.arrayLength);
        }
    };
};

///
struct GeometryInterface {
    using ViewType = GeometryResource*;
};

///
struct LightsInterface {
    using ViewType = LightsResource*;
};

/// Base class of `PassSource` and `PassSink`
class PassInterfaceBase {
    using Variant = std::variant<BufferInterface, TextureInterface,
                                 GeometryInterface, LightsInterface>;

public:
    virtual ~PassInterfaceBase() = default;

    /// \Returns the underlying `*Interface` variant
    Variant& get() { return _interface; }

    /// \overload
    Variant const& get() const { return _interface; }

    /// \Returns the underlying `Interface` of type \p T
    template <InterfaceType T>
    T& get() {
        return std::get<T>(get());
    }

    /// \overload
    template <InterfaceType T>
    T const& get() const {
        return std::get<T>(get());
    }

    ///
    template <InterfaceType T>
    typename T::ViewType* handle() const {
        return static_cast<typename T::ViewType*>(_handle);
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

protected:
    template <InterfaceType T>
    PassInterfaceBase(Pass* pass, std::string name,
                      typename T::ViewType* handle, T interface):
        _name(std::move(name)),
        _handle(handle),
        _interface(std::move(interface)),
        _pass(pass) {}

private:
    std::string _name;
    void* _handle;
    Variant _interface;
    Pass* _pass = nullptr;
    Resource* _resource = nullptr;
};

/// Resource input of a pass
class PassSink: public PassInterfaceBase {
public:
    template <InterfaceType T>
    explicit PassSink(Pass* pass, std::string name,
                      typename T::ViewType* handle, T interface):
        PassInterfaceBase(pass, std::move(name), handle, std::move(interface)) {
    }

    /// \Returns the source that is linked to this sink
    /// \Note `addSink` on \p source must be called manually
    PassSource* source() const { return _source; }

    /// \Returns the pass whose source is linked to this sink or null if no
    /// source is linked
    Pass* getSourcePass() const;

    ///
    void setSource(PassSource* source) { _source = source; }

private:
    PassSource* _source = nullptr;
};

/// Resource output of a pass
class PassSource: public PassInterfaceBase {
public:
    template <InterfaceType T>
    explicit PassSource(Pass* pass, std::string name,
                        typename T::ViewType* handle, T interface):
        PassInterfaceBase(pass, std::move(name), handle, std::move(interface)) {
    }

    /// \Returns the source that is linked to this sink
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
    template <InterfaceType T>
    explicit RenderGraphSink(Pass* pass, std::string name,
                             typename T::ViewType* handle, T interface):
        PassSink(pass, std::move(name), handle, std::move(interface)),
        src(nullptr, name, handle, interface) {
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
    template <InterfaceType T>
    explicit RenderGraphSource(Pass* pass, std::string name,
                               typename T::ViewType* handle, T interface):
        PassSource(pass, name, handle, interface),
        snk(nullptr, name, handle, interface) {
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

#endif // BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
