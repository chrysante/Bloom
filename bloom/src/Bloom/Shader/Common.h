#ifndef BLOOM_SHADER_COMMON_H
#define BLOOM_SHADER_COMMON_H

#include "Bloom/Core/Base.h"

#ifdef BLOOM_METAL

#include <metal_stdlib>
#define BLOOM_SHADER_CONSTANT constant

namespace vml = metal;

#else

#include <vml/vml.hpp>
#define BLOOM_SHADER_CONSTANT constexpr

#endif

#endif // BLOOM_SHADER_COMMON_H
