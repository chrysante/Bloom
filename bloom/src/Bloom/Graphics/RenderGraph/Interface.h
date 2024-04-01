#ifndef BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H

#include <span>
#include <string>
#include <variant>

#include <range/v3/view.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"

namespace bloom::rg {

class SceneGeometry;
class SceneLights;

///
struct BufferInterface {
    using ViewType = BufferHandle;

    ///
    std::string name;

    ///
    BufferHandle* handle = nullptr;

    ///
    size_t size;
};

/// Size descriptor that supports absolute and relative values
struct TextureSize {
    struct Absolute {
        size_t size;
    };
    struct RelativeToScreen {
        double factor = 1.0;
    };

    using Value = std::variant<Absolute, RelativeToScreen>;

    Value width;
    Value height;
    Value depth;

    /// Size descriptor that describes the screen size
    static TextureSize const ScreenSize;
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
};

///
struct SceneGeometryInterface {
    using ViewType = SceneGeometry*;

    ///
    std::string name;

    ///
    SceneGeometry** handle;
};

///
struct SceneLightsInterface {
    using ViewType = SceneLights*;

    ///
    std::string name;

    ///
    SceneLights** handle;
};

/// Base class of `PassSource` and `PassSink`
class PassInterfaceBase {
    using Variant = std::variant<BufferInterface, TextureInterface,
                                 SceneGeometryInterface, SceneLightsInterface>;

public:
    /// \Returns the underlying `*Interface` variant
    Variant& get() { return _interface; }

    /// \overload
    Variant const& get() const { return _interface; }

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

protected:
    PassInterfaceBase(Pass* pass, InterfaceType auto interface):
        _pass(pass), _interface(std::move(interface)) {}

private:
    Pass* _pass;
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
