#ifndef BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H

#include <string>
#include <variant>

#include <vml/vml.hpp>

#include "Bloom/GPU/HardwarePrimitives.h"

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

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_INTERFACE_H
