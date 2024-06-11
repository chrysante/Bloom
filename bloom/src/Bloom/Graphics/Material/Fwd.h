#ifndef BLOOM_GRAPHICS_MATERIAL_FWD_H
#define BLOOM_GRAPHICS_MATERIAL_FWD_H

#include <cassert>
#include <string_view>

namespace bloom {

class Material;
class MaterialInstance;

/// Declaration order matters because parameter descriptors are sorted based on
/// their data type
enum class MaterialDataType {
    Float,
    Float2,
    Float3,
    Float4,
    Texture2D,
    TextureCube
};

/// \Returns the size of \p type
size_t layoutSize(MaterialDataType type);

/// Global buffer and texture binding points used for shading

/// Different material shading models
enum class MaterialShadingModel { Unlit, DefaultLit };

/// Different material transparency modes
enum class MaterialTransparency { Opaque, Masked };

/// Types of shaders used in materials
enum class MaterialShaderType {
    ShadingVertex,
    ShadingFragment,
    ShadowVertex,
    ShadowFragment
};

struct MaterialSourceData;

///
struct MaterialBindingPoint {
    enum Type { Buffer, Texture };
    Type type;
    int index;
};

/// Global buffer and texture content types
enum class GlobalMaterialBindingType {
    Camera,
    Transform,
    Geometry,
    Lights,
    LightSpaceTransforms,
    ShadowMaps,
    MaterialParameterBuffer,
    GeometryIDBuffer,
    SpecularTexture,
    BRDFTexture,
    LAST = BRDFTexture
};

/// \Returns the shader binding points for the different combinations of shader
/// types and content types \Note This is inline because the material compiler
/// doesn't link against the main library
inline MaterialBindingPoint getGlobalMaterialBindingPoint(
    MaterialShaderType shaderType, GlobalMaterialBindingType bindingType) {
    using enum GlobalMaterialBindingType;
    using enum MaterialBindingPoint::Type;
    switch (shaderType) {
    case MaterialShaderType::ShadingVertex:
        [[fallthrough]];
    case MaterialShaderType::ShadowVertex:
        switch (bindingType) {
        case Camera:
            return { Buffer, 0 };
        case Transform:
            return { Buffer, 1 };
        case Geometry:
            return { Buffer, 2 };
        case MaterialParameterBuffer:
            return { Buffer, 4 };
        default:
            assert(false);
        }
    case MaterialShaderType::ShadingFragment:
        [[fallthrough]];
    case MaterialShaderType::ShadowFragment:
        switch (bindingType) {
        case Camera:
            return { Buffer, 0 };
        case Transform:
            assert(false);
        case Geometry:
            assert(false);
        case Lights:
            return { Buffer, 1 };
        case LightSpaceTransforms:
            return { Buffer, 2 };
        case ShadowMaps:
            return { Texture, 0 };
        case MaterialParameterBuffer:
            return { Buffer, 4 };
        case GeometryIDBuffer:
            return { Buffer, 5 };
        case SpecularTexture:
            return { Texture, 4 };
        case BRDFTexture:
            return { Texture, 5 };
        }
    }
}

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_FWD_H
