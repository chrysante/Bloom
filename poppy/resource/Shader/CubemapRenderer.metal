#include <metal_stdlib>

using namespace metal;

// clang-format off
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
}; // clang-format on

struct CRVSOut {
    float4 position [[position]];
    float3 localPosition;
    float2 uv;
};

vertex CRVSOut cubeRendererVS(float4x4 device const& camera [[buffer(0)]],
                              uint32_t vertexID [[vertex_id]])
{
    float3 position = vertices[vertexID];
    return { .position = camera * float4(position, 1),
             .localPosition = position };
}

static float2 sampleSphericalMap(float3 v)
{
    float2 invAtan = { 0.1591, 0.3183 };
    float2 uv = { atan2(v.y, v.x), asin(v.z) };
    uv *= invAtan;
    uv += 0.5;
    uv.y = 1 - uv.y;
    return uv;
}

fragment float4 cubemapRendererFS(CRVSOut in [[stage_in]],
                                  texture2d<float> tex [[texture(0)]])
{
    float2 uv = sampleSphericalMap(normalize(in.localPosition));
    constexpr sampler samplerState(min_filter::linear, mag_filter::linear);
    return tex.sample(samplerState, uv);
}

static float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

static float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

constant float PI = 3.14159265359;

static float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    // from spherical coordinates to cartesian coordinates
    float3 H = {
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta,
    };
    // from tangent-space vector to world-space sample vector
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) :
                                   float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

static float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    float PI = 3.14159265359;
    denom = PI * denom * denom;
    return num / denom;
}

constant uint SampleCount = 64 * 1024u;

fragment float4 specularIBLRendererFS(CRVSOut in [[stage_in]],
                                      float device const& roughness
                                      [[buffer(0)]],
                                      texturecube<float> source [[texture(0)]])
{
    float3 N = normalize(in.localPosition);
    float3 R = N;
    float3 V = R;
    float totalWeight = 0.0;
    float3 prefilteredColor{};
    for (uint i = 0u; i < SampleCount; ++i) {
        float2 Xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) {
            continue;
        }
        float NdotH = dot(N, H);
        float HdotV = dot(H, V);
        float D = distributionGGX(NdotH, roughness);
        float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;
        float saTexel =
            4.0 * PI / (6.0 * source.get_width() * source.get_height());
        float saSample = 1.0 / (float(SampleCount) * pdf + 0.0001);
        float mipLevel = roughness == 0.0 ? 0.0 :
                                            0.5 * log2(saSample / saTexel);
        constexpr sampler s(min_filter::linear, mag_filter::linear,
                            mip_filter::linear);
        prefilteredColor += source.sample(s, L, level(mipLevel)).rgb * NdotL;
        totalWeight += NdotL;
    }
    return float4(prefilteredColor / totalWeight, 1.0);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

static float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V = {
        sqrt(1.0 - NdotV * NdotV),
        0.0,
        NdotV,
    };
    float A = 0.0;
    float B = 0.0;
    float3 N = float3(0.0, 0.0, 1.0);
    for (uint i = 0u; i < SampleCount; ++i) {
        float2 Xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if (NdotL <= 0.0) {
            continue;
        }
        float G = GeometrySmith(N, V, L, roughness);
        float G_Vis = (G * VdotH) / (NdotH * NdotV);
        float Fc = pow(1.0 - VdotH, 5.0);
        A += (1.0 - Fc) * G_Vis;
        B += Fc * G_Vis;
    }
    A /= float(SampleCount);
    B /= float(SampleCount);
    return float2(A, B);
}

kernel void IntegrateBRDFKernel(texture2d<float, access::write> dest
                                [[texture(0)]],
                                uint2 position [[thread_position_in_grid]])
{
    float2 uv = (float2(position) + float2(0.5)) /
                float2(dest.get_width(), dest.get_height());
    uv.y = 1 - uv.y;
    float2 integratedBRDF = IntegrateBRDF(uv.x, uv.y);
    dest.write(float4(integratedBRDF, 0, 1), position);
}
