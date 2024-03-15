#ifndef BLOOM_GRAPHICS_SHADERBASE_H
#define BLOOM_GRAPHICS_SHADERBASE_H

#include "Bloom/Core/Base.h"

#ifdef BLOOM_METAL

#include <metal_stdlib>
#define BLOOM_SHADER_CONSTANT constant

#else

#include <mtl/mtl.hpp>
namespace metal = mtl;
#define BLOOM_SHADER_CONSTANT constexpr

#endif

#endif // BLOOM_GRAPHICS_SHADERBASE_H
