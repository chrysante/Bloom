#pragma once

#include "Bloom/Core/Base.hpp"

#ifdef BLOOM_METAL

#include <metal_stdlib>
#define BLOOM_SHADER_CONSTANT constant

#else

#include <mtl/mtl.hpp>
namespace metal = mtl;
#define BLOOM_SHADER_CONSTANT constexpr

#endif
