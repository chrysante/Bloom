#include <metal_stdlib>

#include "Shader/MaterialParameters.h"
#include "Shader/Vertex.h"
#include "Shader/SharedTypes.h"
#include "Shader/PBRShader.h"

using namespace metal;
using namespace bloom;

struct MainPassData {
    float4 positionCS [[position]];
    float3 normalWS;
    float3 positionWS;
    float3 vertexColor;
    float2 textureCoords;
};

struct MainPassFragmentData {
    float4 color [[color(0)]];
    uint32_t entityID [[color(1)]];
};

vertex MainPassData rgMainPassVS(CameraInput device const& cameraBuffer
                                 [[buffer(0)]],
                                 Vertex3D device const* vertices [[buffer(1)]],
                                 float4x4 device const& transform [[buffer(2)]],
                                 unsigned vertexID [[vertex_id]])
{
    Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 vertexNormalMS = float4(v.normal, 0);
    float4 vertexPositionWS = transform * vertexPositionMS;
    return {
        .positionCS = cameraBuffer.camera * vertexPositionWS,
        .normalWS = (transform * vertexNormalMS).xyz,
        .positionWS = vertexPositionWS.xyz,
        .vertexColor = v.color.rgb,
        .textureCoords = v.textureCoordinates[0]
    };
}

fragment MainPassFragmentData rgMainPassFS(
    MainPassData in [[stage_in]],
    CameraInput device const& cameraBuffer [[buffer(0)]],
    LightBufferContents device const& lights [[buffer(1)]],
    float4x4 device const* lightSpaceTransforms [[buffer(2)]],
    ShadowRenderMetadata device const& shadowMetadata [[buffer(3)]],
    MaterialParameters device const& materialParams [[buffer(4)]],
    uint32_t device const& entityID [[buffer(5)]],
    texture2d_array<float> shadowMaps [[texture(0)]])
{
    PBRData pbr{ .albedo = materialParams.baseColor,
                 .metallic = materialParams.metallic,
                 .roughness = materialParams.roughness,
                 .ao = 1 };
    float3 color =
        PBRForwardShader(pbr, in.positionWS, normalize(in.normalWS.xyz),
                         cameraBuffer.cameraPosition, lightSpaceTransforms,
                         lights, shadowMetadata, shadowMaps);
    return { float4(color, 1), entityID };
}
