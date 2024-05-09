#include <metal_stdlib>

#include "Bloom/Shader/SharedTypes.h"

using namespace metal;
using namespace bloom;

static float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

static float3 reinhard(float3 x) { return x / (x + 1.0); }

static float3 tonemap(float3 colorIn, ToneMappingFunction function)
{
    switch (function) {
    case ToneMappingFunction::ACES:
        return ACESFilm(colorIn);
    case ToneMappingFunction::reinhard:
        return reinhard(colorIn);
    default:
        return colorIn;
    }
}

static float3 gammaCorrect(float3 colorIn) { return pow(colorIn, 1.0 / 2.2); }

kernel void rgPostprocess(PostProcessParameters device const& params
                          [[buffer(0)]],
                          texture2d<float, access::write> dest [[texture(0)]],
                          texture2d<float, access::read> rawColorTex
                          [[texture(1)]],
                          uint2 position [[thread_position_in_grid]])
{
    float3 rawColor = rawColorTex.read(position).rgb;
    float3 toneMapped = tonemap(rawColor, params.toneMappingFunction);
    float3 gammaCorrected = gammaCorrect(toneMapped);
    dest.write(float4(gammaCorrected, 1), position);
}
