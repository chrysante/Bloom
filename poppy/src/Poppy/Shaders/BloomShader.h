#pragma once

#include <metal_stdlib>

namespace bloom {
	
	metal::half4 upsampleTent(metal::texture2d<half, metal::access::sample> tex,
							  metal::float2 uv,
							  metal::float2 texelSize,
							  metal::float4 sampleScale);
	
};
