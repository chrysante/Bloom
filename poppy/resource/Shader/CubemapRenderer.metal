#include <metal_stdlib>

using namespace metal;

constant float3 vertices[] = {
    float3(-1)
};

vertex float4 cubemapRendererVS(float4x4 device const& camera [[buffer(0)]],
                                uint32_t vertexID [[vertex_id]]) {
    return float4();
}

fragment float4 cubemapRendererFS(float4 position [[stage_in]],
                                  texture2d<float> tex [[texture(0)]]) {
    constexpr sampler samplerState(min_filter::linear,
                                   mag_filter::linear);
    return tex.sample(samplerState, position.xy);
}
