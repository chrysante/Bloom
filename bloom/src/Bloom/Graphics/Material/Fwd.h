#ifndef BLOOM_GRAPHICS_MATERIAL_FWD_H
#define BLOOM_GRAPHICS_MATERIAL_FWD_H

#include <string_view>

namespace bloom {

class Material;

enum class MaterialShadingModel { Unlit, DefaultLit, LAST = DefaultLit };

inline constexpr std::string_view MaterialVertexShaderName =
    "materialVertexShader";
inline constexpr std::string_view MaterialFragmentShaderName =
    "materialFragmentShader";

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIAL_FWD_H
