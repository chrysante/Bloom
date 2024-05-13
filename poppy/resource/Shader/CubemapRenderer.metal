#include <metal_stdlib>

using namespace metal;

constant float3 vertices[] = {
    // +X
    float3(1, -1, -1),
    float3(1, -1,  1),
    float3(1,  1, -1),
    float3(1,  1,  1),
    // -X
    float3(-1, -1, -1),
    float3(-1, -1,  1),
    float3(-1,  1, -1),
    float3(-1,  1,  1),
    // +Y
    float3(-1, 1, -1),
    float3(-1, 1,  1),
    float3( 1, 1, -1),
    float3( 1, 1,  1),
    // -Y
    float3(-1, -1, -1),
    float3(-1, -1,  1),
    float3( 1, -1, -1),
    float3( 1, -1,  1),
    // +Z
    float3(-1, -1, 1),
    float3(-1,  1, 1),
    float3( 1, -1, 1),
    float3( 1,  1, 1),
    // -Z
    float3(-1, -1, -1),
    float3(-1,  1, -1),
    float3( 1, -1, -1),
    float3( 1,  1, -1),
};

struct CRVSOut {
    float4 position [[position]];
    float3 localPosition;
    float2 uv;
};

vertex CRVSOut cubemapRendererVS(float4x4 device const& camera [[buffer(0)]],
                                 uint32_t vertexID [[vertex_id]]) {
    float3 position = vertices[vertexID];
    return {
        .position = camera * float4(position, 1),
        .localPosition = position
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
