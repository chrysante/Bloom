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
    float3 tangentWS;
    float3 binormalWS;
    float3 positionWS;
    float3 vertexColor;
    float2 textureCoords;
};

struct MainPassFragmentData {
    float4 color [[color(0)]];
    uint32_t entityID [[color(1)]];
};

// NOTE: The buffer and texture bindings in this file are hard coded. The binding points are defined in "Material/Fwd.h"

vertex MainPassData rgMainPassVS(CameraInput device const& cameraBuffer
                                 [[buffer(0)]],
                                 Vertex3D device const* vertices [[buffer(2)]],
                                 float4x4 device const& transform [[buffer(1)]],
                                 unsigned vertexID [[vertex_id]])
{
    Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 vertexNormalMS = float4(v.normal, 0);
    float4 vertexTangentMS = float4(v.tangent, 0);
    float4 vertexBitangentMS = float4(v.binormal, 0);
    float4 vertexPositionWS = transform * vertexPositionMS;
    return {
        .positionCS = cameraBuffer.camera * vertexPositionWS,
        .normalWS = (transform * vertexNormalMS).xyz,
        .tangentWS = (transform * vertexTangentMS).xyz,
        .binormalWS = (transform * vertexBitangentMS).xyz,
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
    uint32_t device const& entityID [[buffer(5)]],
    texture2d_array<float> shadowMaps [[texture(0)]])
{
    return { float4(in.textureCoords, 0, 1), entityID };
}
