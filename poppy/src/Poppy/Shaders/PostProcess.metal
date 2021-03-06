#include <metal_stdlib>

#include "PostProcess.h"
#include "BloomShader.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.hpp"

using namespace metal;
using namespace bloom;

vertex PPRasterOutput postprocessVS(uint const vertexID [[ vertex_id ]]) {
	return postProcessQuadVertices[vertexID];
}

[[maybe_unused]] static float3 ACESFilm(float3 x) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[[maybe_unused]] static float3 reinhard(float3 x) {
	return x / (x + 1.0);
}

static float3 readBloomTexture(texture2d<half, access::sample> tex, float2 uv, float scale) {
	float2 const texSize = float2(tex.get_width(), tex.get_height());
	float2 const texelSize = 1 / (texSize - 1);
	
	return float3(upsampleTent(tex, uv, texelSize, scale).rgb);
}

kernel void postprocess(PostProcessParameters device const& params  [[ buffer(0) ]],
						texture2d<float, access::write> destination [[ texture(0) ]],
						texture2d<float, access::read>  rawColor    [[ texture(1) ]],
						texture2d<half, access::sample> bloomTex    [[ texture(2) ]],
						uint2 position                              [[ thread_position_in_grid ]])
{
	float2 const destSize = float2(destination.get_width(), destination.get_height());
	float2 const uv = float2(position) / (destSize - 1);
	
	float3 color = rawColor.read(position).rgb;
	
	// Bloom
	if (params.bloom.enabled) {
		float3 const bloomValue = readBloomTexture(bloomTex, uv, params.bloom.scale);
		if (params.bloom.physicallyCorrect) {
			color = mix(color, bloomValue, params.bloom.contribution);
		}
		else {
			color += params.bloom.intensity * bloomValue;
		}
	}
	
	// Tonemapping
	float3 toneMapped;
	switch (params.tonemapping) {
		case ToneMapping::ACES:
			toneMapped = ACESFilm(color);
			break;
		case ToneMapping::reinhard:
			toneMapped = reinhard(color);
			break;
		default:
			toneMapped = color;
			break;
	}
	
	// gamma correction
	float3 const corrected = pow(toneMapped, 1.0 / 2.2);
	
	destination.write(float4(corrected, 1), position);
}
