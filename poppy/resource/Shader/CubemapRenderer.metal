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
    // -Y
    float3(-1, -1, -1),
    float3(-1, -1,  1),
    float3( 1, -1, -1),
    float3( 1, -1,  1),
    // +Y
    float3(-1, 1, -1),
    float3(-1, 1,  1),
    float3( 1, 1, -1),
    float3( 1, 1,  1),
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

vertex CRVSOut cubeRendererVS(float4x4 device const& camera [[buffer(0)]],
                                 uint32_t vertexID [[vertex_id]]) {
    float3 position = vertices[vertexID];
    return {
        .position = camera * float4(position, 1),
        .localPosition = position
    };
}

static float2 sampleSphericalMap(float3 v) {
    float2 invAtan = { 0.1591, 0.3183 };
    float2 uv = { atan2(v.y, v.x), asin(v.z) };
    uv *= invAtan;
    uv += 0.5;
    uv.y = 1 - uv.y;
    return uv;
}

fragment float4 cubemapRendererFS(CRVSOut in [[stage_in]],
                                  texture2d<float> tex [[texture(0)]]) {
    float2 uv = sampleSphericalMap(normalize(in.localPosition));
    constexpr sampler samplerState(min_filter::linear,
                                   mag_filter::linear);
    return tex.sample(samplerState, uv);
}

fragment float4 irradianceRendererFS(CRVSOut in [[stage_in]],
                                  texturecube<float> source [[texture(0)]]) {
    float3 normal = normalize(in.localPosition);
    float3 right = normalize(cross({ 0.0, -1.0, 0.0 }, normal));
    float3 up    = normalize(cross(normal, right));
    float3 irradiance = {};
    float sampleDelta = 0.025;
    int nrSamples = 0;
    const float PI = 3.14159265359;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Spherical to cartesian (in tangent space)
            float3 tangentSample = {
                sin(theta) * cos(phi), 
                sin(theta) * sin(phi),
                cos(theta)
            };
            // Tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
            constexpr sampler s(min_filter::linear, mag_filter::linear);
            float3 sampleColor = source.sample(s, sampleVec).rgb;
            irradiance += sampleColor * dot(sampleVec, normal);
            ++nrSamples;
        }
    }
    float3 final = PI * irradiance / float(nrSamples);
    return float4(final, 1);
}
