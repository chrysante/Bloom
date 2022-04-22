#pragma once

#include "Bloom/Core/Base.hpp"

#ifdef BLOOM_METAL
#include <metal_stdlib>
#else
#include <mtl/mtl.hpp>
namespace metal = mtl;
#endif
