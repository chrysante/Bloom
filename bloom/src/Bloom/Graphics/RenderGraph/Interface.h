#ifndef BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H

#include <span>
#include <string>
#include <variant>

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
    std::string name;

    ///
    BufferHandle* handle = nullptr;

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

    using Value = std::variant<Absolute, RelativeToScreen>;

    Value width;
    Value height;
    Value depth;

    /// Size descriptor that describes the screen size
    static TextureSize const ScreenSize;

    bool operator==(TextureSize const&) const = default;

    struct Hash {
        size_t operator()(TextureSize const& size) const {
            size_t seed = 0;
            for (auto mem: { &TextureSize::width, &TextureSize::height,
                             &TextureSize::depth })
            {
                // clang-format off
                utl::hash_combine_seed(seed, std::visit(utl::overload{
                    [](Absolute a) { return std::hash<size_t>{}(a.size); },
                    [](RelativeToScreen r) {
                        return std::hash<double>{}(r.factor);
                    },
                }, size.*mem)); // clang-format on
            }
            return seed;
        }
    };
};

constexpr TextureSize TextureSize::ScreenSize = {
    TextureSize::RelativeToScreen{ 1.0 },
    TextureSize::RelativeToScreen{ 1.0 },
    TextureSize::Absolute{ 1 },
};

///
struct TextureInterface {
    using ViewType = TextureHandle;

    ///
    std::string name;

    ///
    TextureHandle* handle = nullptr;

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

    ///
    std::string name;

    ///
    GeometryResource** handle;
};

///
struct LightsInterface {
    using ViewType = LightsResource*;

    ///
    std::string name;

    ///
    LightsResource** handle;
};

/// Base class of `PassSource` and `PassSink`
class PassInterfaceBase {
    using Variant = std::variant<BufferInterface, TextureInterface,
                                 GeometryInterface, LightsInterface>;

public:
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

    /// \Returns `std::visit(vis, get())`
    decltype(auto) visit(auto&& vis) const { return std::visit(vis, get()); }

    /// \Returns the name of this interface
    std::string const& name() const {
        return visit([](auto& interface) -> auto& { return interface.name; });
    }

    /// \Returns the pass the which this interface belongs
    Pass* pass() { return _pass; }

    /// \overload
    Pass const* pass() const { return _pass; }

    /// \Returns the backing resource or null if none exists
    Resource* resource() const { return _resource; }

    /// Assigns the backing resource
    void setResource(Resource* resource) { _resource = resource; }

protected:
    PassInterfaceBase(Pass* pass, InterfaceType auto interface):
        _pass(pass), _interface(std::move(interface)) {}

private:
    Pass* _pass = nullptr;
    Resource* _resource = nullptr;
    Variant _interface;
};

/// Resource input of a pass
class PassSink: public PassInterfaceBase {
public:
    explicit PassSink(Pass* pass, InterfaceType auto interface):
        PassInterfaceBase(pass, std::move(interface)) {}

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

/// Resource input of a pass
class PassSource: public PassInterfaceBase {
public:
    explicit PassSource(Pass* pass, InterfaceType auto interface):
        PassInterfaceBase(pass, std::move(interface)) {}

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

/// # Inline implementation

inline Pass* PassSink::getSourcePass() const {
    return source() ? source()->pass() : nullptr;
}

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
