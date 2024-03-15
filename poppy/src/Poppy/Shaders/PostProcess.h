#ifndef POPPY_SHADERS_POSTPROCESS_H
#define POPPY_SHADERS_POSTPROCESS_H

#include <metal_stdlib>

namespace bloom {

struct PPRasterOutput {
    metal::float4 position [[position]];
    metal::float2 screenCoords;
};

constant PPRasterOutput postProcessQuadVertices[] = {
    PPRasterOutput{ float4(-1, -1, 0, 1), float2(0, 1) },
    PPRasterOutput{ float4(1, -1, 0, 1), float2(1, 1) },
    PPRasterOutput{ float4(-1, 1, 0, 1), float2(0, 0) },

    PPRasterOutput{ float4(1, -1, 0, 1), float2(1, 1) },
    PPRasterOutput{ float4(1, 1, 0, 1), float2(1, 0) },
    PPRasterOutput{ float4(-1, 1, 0, 1), float2(0, 0) }
};

} // namespace bloom

#endif // POPPY_SHADERS_POSTPROCESS_H
