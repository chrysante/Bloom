#ifndef POPPY_SHADERS_BLOOMSHADER_H
#define POPPY_SHADERS_BLOOMSHADER_H

#include <metal_stdlib>

namespace bloom {

metal::half4 upsampleTent(metal::texture2d<half, metal::access::sample> tex,
                          metal::float2 uv, metal::float2 texelSize,
                          metal::float4 sampleScale);

};

#endif // POPPY_SHADERS_BLOOMSHADER_H
