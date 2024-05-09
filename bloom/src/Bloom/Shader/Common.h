#ifndef BLOOM_SHADER_COMMON_H
#define BLOOM_SHADER_COMMON_H

#ifdef __METAL_VERSION__

#include <metal_stdlib>
#define BLOOM_SHADER_CONSTANT constant

namespace vml = metal;

#else

#include <vml/vml.hpp>
#define BLOOM_SHADER_CONSTANT static constexpr

#endif

#endif // BLOOM_SHADER_COMMON_H
