#ifndef BLOOM_SHADER_SHAREDTYPES_H
#define BLOOM_SHADER_SHAREDTYPES_H

#include "Bloom/Shader/Common.h"

namespace bloom {

///
struct CameraInput {
    metal::float4x4 camera;
    metal::float3 cameraPosition;
};

/// Different tone mapping functions
enum class ToneMappingFunction { ACES, reinhard };

/// Parameter structure for the bloom shaders
struct BloomParameters {
    bool physicallyCorrect = true;
    metal::float3 curve;
    float contribution;
    float intensity;
    float scale;
    float threshold;
    float clamp;
};

/// Parameter structure for the post processing shader
struct PostProcessParameters {
    ToneMappingFunction toneMappingFunction;
};

} // namespace bloom

#endif // BLOOM_SHADER_SHAREDTYPES_H
