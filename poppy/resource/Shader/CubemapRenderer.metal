#include <metal_stdlib>

using namespace metal;

struct CRVertex {
    float3 position;
    float2 uv;
};

constant CRVertex vertices[] = {
    { float3(1, -1, -1), float2(0, 0) },
    { float3(1, -1,  1), float2(0, 1) },
    { float3(1,  1, -1), float2(1, 0) },
    { float3(1,  1,  1), float2(1, 1) },
};

struct CRVSOut {
    float4 position [[position]];
    float3 localPosition;
    float2 uv;
};

vertex CRVSOut cubemapRendererVS(float4x4 device const& camera [[buffer(0)]],
                                uint32_t vertexID [[vertex_id]]) {
    CRVertex v = vertices[vertexID];
    return {
        .position = camera * float4(v.position, 1),
        .localPosition = v.position,
        .uv = v.uv
    };
}

static float2 sampleSphericalMap(float3 v) {
    float2 invAtan = { 0.1591, 0.3183 };
    float2 uv = { atan2(v.z, v.x), asin(v.y) };
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

fragment float4 cubemapRendererFS(CRVSOut in [[stage_in]],
                                  texture2d<float> tex [[texture(0)]]) {
    float2 uv = sampleSphericalMap(normalize(in.localPosition));
    constexpr sampler samplerState(min_filter::linear,
                                   mag_filter::linear);
    return tex.sample(samplerState, uv);
}
