#include <metal_stdlib>

#include "BloomShader.h"

#include "Bloom/Graphics/Renderer/ShaderParameters.h"

using namespace metal;
using namespace bloom;

constexpr sampler bilinSampler(coord::normalized,
							   address::clamp_to_zero,
							   filter::linear);

static half3 safeHDR(half3 color) {
	return min(color, HALF_MAX);
}

static half4 safeHDR(half4 color) {
	return min(color, HALF_MAX);
}

static half4 downsampleBox13Tap(texture2d<half, access::sample> tex,
								float2 uv,
								float2 texelSize)
{
	half4 const A = tex.sample(bilinSampler, uv + texelSize * float2(-1.0, -1.0));
	half4 const B = tex.sample(bilinSampler, uv + texelSize * float2( 0.0, -1.0));
	half4 const C = tex.sample(bilinSampler, uv + texelSize * float2( 1.0, -1.0));
	half4 const D = tex.sample(bilinSampler, uv + texelSize * float2(-0.5, -0.5));
	half4 const E = tex.sample(bilinSampler, uv + texelSize * float2( 0.5, -0.5));
	half4 const F = tex.sample(bilinSampler, uv + texelSize * float2(-1.0,  0.0));
	half4 const G = tex.sample(bilinSampler, uv                                 );
	half4 const H = tex.sample(bilinSampler, uv + texelSize * float2( 1.0,  0.0));
	half4 const I = tex.sample(bilinSampler, uv + texelSize * float2(-0.5,  0.5));
	half4 const J = tex.sample(bilinSampler, uv + texelSize * float2( 0.5,  0.5));
	half4 const K = tex.sample(bilinSampler, uv + texelSize * float2(-1.0,  1.0));
	half4 const L = tex.sample(bilinSampler, uv + texelSize * float2( 0.0,  1.0));
	half4 const M = tex.sample(bilinSampler, uv + texelSize * float2( 1.0,  1.0));

	half2 const div = (1.0 / 4.0) * half2(0.5, 0.125);

	half4 o  = (D + E + I + J) * div.x;
	      o += (A + B + G + F) * div.y;
	      o += (B + C + H + G) * div.y;
	      o += (F + G + L + K) * div.y;
	      o += (G + H + M + L) * div.y;

	return o;
}

// 9-tap bilinear upsampler (tent filter)
half4 bloom::upsampleTent(texture2d<half, access::sample> tex, float2 uv, float2 texelSize, float4 sampleScale) {
	float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0) * sampleScale;

	half4 s;
	s =  tex.sample(bilinSampler, uv - d.xy);
	s += tex.sample(bilinSampler, uv - d.wy) * 2.0;
	s += tex.sample(bilinSampler, uv - d.zy);

	s += tex.sample(bilinSampler, uv + d.zw) * 2.0;
	s += tex.sample(bilinSampler, uv       ) * 4.0;
	s += tex.sample(bilinSampler, uv + d.xw) * 2.0;

	s += tex.sample(bilinSampler, uv + d.zy);
	s += tex.sample(bilinSampler, uv + d.wy) * 2.0;
	s += tex.sample(bilinSampler, uv + d.xy);

	return s * (1.0 / 16.0);
}

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
template <typename T>
static vec<T, 3> quadraticThreshold(vec<T, 3> color, T threshold, vec<T, 3> curve) {
	// Pixel brightness
	T const br = max(max(color.r, color.g), color.b);

	// Under-threshold part
	T rq = clamp(br - curve.x, T(0), curve.y);
	rq = curve.z * rq * rq;

	// Combine and apply the brightness response curve
	color *= max(rq, br - threshold) / max(br, T(1e-4));

	return color;
}

kernel void bloomPrefilter(texture2d<half, access::sample> source [[ texture(0) ]],
						   texture2d<half, access::write>  dest   [[ texture(1) ]],
						   BloomParameters device const&   params [[ buffer(0) ]],
						   uint2 position                         [[ thread_position_in_grid ]])
{
	float2 const destSize = float2(dest.get_width(), dest.get_height());
	float2 const uv = float2(position) / (destSize - 1);
	float2 const texelSize = 1 / (destSize - 1);
	
	half3 sample = safeHDR(downsampleBox13Tap(source, uv, texelSize).rgb);
	sample = min(sample, params.clamp);
	
	if (!params.physicallyCorrect) {
		sample = quadraticThreshold(sample, (half)params.threshold, (half3)params.curve);
	}
	dest.write(half4(sample, 1), position);
}


kernel void bloomDownsample(texture2d<half, access::sample> source [[ texture(0) ]],
							texture2d<half, access::write>   dest  [[ texture(1) ]],
							uint2 position                         [[ thread_position_in_grid ]])
{
	float2 const destSize = float2(dest.get_width(), dest.get_height());
	float2 const uv = float2(position) / (destSize - 1);
	float2 const texelSize = 1 / (destSize - 1);
	
	half4 const value = downsampleBox13Tap(source, uv, texelSize);

	dest.write(value, position);
}

kernel void bloomUpsample(texture2d<half, access::sample> sourceSmall [[ texture(0) ]],
						  texture2d<half, access::read>   sourceBig   [[ texture(1) ]],
						  texture2d<half, access::write>  dest        [[ texture(2) ]],
						  BloomParameters device const&   params      [[ buffer(0) ]],
						  uint2 position                              [[ thread_position_in_grid ]])
{
	float2 const destSize = float2(dest.get_width(), dest.get_height());
	float2 const sourceSmallSize = float2(sourceSmall.get_width(), sourceSmall.get_height());
	float2 const uv = float2(position) / (destSize - 1);
	float2 const texelSize = 1 / (sourceSmallSize - 1);
	
	half4 const upsampledValue = upsampleTent(sourceSmall, uv, texelSize, params.scale);
	half4 const big = sourceBig.read(position);
	
	if (params.physicallyCorrect) {
		dest.write(mix(big, upsampledValue, params.contribution), position);
	}
	else {
		dest.write(upsampledValue + big, position);
	}
}

