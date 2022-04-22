#pragma once

#include "Bloom/Core/Base.hpp"
#include "ResourceHandle.hpp"

namespace bloom {
	
	
	enum class CompareFunction {
		never        = 0,
		less         = 1,
		equal        = 2,
		lessEqual    = 3,
		greater      = 4,
		notEqual     = 5,
		greaterEqual = 6,
		always       = 7,
	};
	
	enum class TriangleFillMode {
		fill = 0, lines = 1
	};
	
	enum class StorageMode {
		Shared = 0,
		Managed = 1,
		Private = 2,
	};
	
	enum class PixelFormat {
		invalid = 0,
		A8Unorm = 1,
		R8Unorm = 10,
		R8Unorm_sRGB = 11,
		R8Snorm = 12,
		R8Uint = 13,
		R8Sint = 14,
		R16Unorm = 20,
		R16Snorm = 22,
		R16Uint = 23,
		R16Sint = 24,
		R16Float = 25,
		RG8Unorm = 30,
		RG8Unorm_sRGB = 31,
		RG8Snorm = 32,
		RG8Uint = 33,
		RG8Sint = 34,
		B5G6R5Unorm = 40,
		A1BGR5Unorm = 41,
		ABGR4Unorm = 42,
		BGR5A1Unorm = 43,
		R32Uint = 53,
		R32Sint = 54,
		R32Float = 55,
		RG16Unorm = 60,
		RG16Snorm = 62,
		RG16Uint = 63,
		RG16Sint = 64,
		RG16Float = 65,
		RGBA8Unorm = 70,
		RGBA8Unorm_sRGB = 71,
		RGBA8Snorm = 72,
		RGBA8Uint = 73,
		RGBA8Sint = 74,
		BGRA8Unorm = 80,
		BGRA8Unorm_sRGB = 81,
		RGB10A2Unorm = 90,
		RGB10A2Uint = 91,
		RG11B10Float = 92,
		RGB9E5Float = 93,
		BGR10A2Unorm = 94,
		RG32Uint = 103,
		RG32Sint = 104,
		RG32Float = 105,
		RGBA16Unorm = 110,
		RGBA16Snorm = 112,
		RGBA16Uint = 113,
		RGBA16Sint = 114,
		RGBA16Float = 115,
		RGBA32Uint = 123,
		RGBA32Sint = 124,
		RGBA32Float = 125,
		BC1_RGBA = 130,
		BC1_RGBA_sRGB = 131,
		BC2_RGBA = 132,
		BC2_RGBA_sRGB = 133,
		BC3_RGBA = 134,
		BC3_RGBA_sRGB = 135,
		BC4_RUnorm = 140,
		BC4_RSnorm = 141,
		BC5_RGUnorm = 142,
		BC5_RGSnorm = 143,
		BC6H_RGBFloat = 150,
		BC6H_RGBUfloat = 151,
		BC7_RGBAUnorm = 152,
		BC7_RGBAUnorm_sRGB = 153,
		PVRTC_RGB_2BPP = 160,
		PVRTC_RGB_2BPP_sRGB = 161,
		PVRTC_RGB_4BPP = 162,
		PVRTC_RGB_4BPP_sRGB = 163,
		PVRTC_RGBA_2BPP = 164,
		PVRTC_RGBA_2BPP_sRGB = 165,
		PVRTC_RGBA_4BPP = 166,
		PVRTC_RGBA_4BPP_sRGB = 167,
		EAC_R11Unorm = 170,
		EAC_R11Snorm = 172,
		EAC_RG11Unorm = 174,
		EAC_RG11Snorm = 176,
		EAC_RGBA8 = 178,
		EAC_RGBA8_sRGB = 179,
		ETC2_RGB8 = 180,
		ETC2_RGB8_sRGB = 181,
		ETC2_RGB8A1 = 182,
		ETC2_RGB8A1_sRGB = 183,
		ASTC_4x4_sRGB = 186,
		ASTC_5x4_sRGB = 187,
		ASTC_5x5_sRGB = 188,
		ASTC_6x5_sRGB = 189,
		ASTC_6x6_sRGB = 190,
		ASTC_8x5_sRGB = 192,
		ASTC_8x6_sRGB = 193,
		ASTC_8x8_sRGB = 194,
		ASTC_10x5_sRGB = 195,
		ASTC_10x6_sRGB = 196,
		ASTC_10x8_sRGB = 197,
		ASTC_10x10_sRGB = 198,
		ASTC_12x10_sRGB = 199,
		ASTC_12x12_sRGB = 200,
		ASTC_4x4_LDR = 204,
		ASTC_5x4_LDR = 205,
		ASTC_5x5_LDR = 206,
		ASTC_6x5_LDR = 207,
		ASTC_6x6_LDR = 208,
		ASTC_8x5_LDR = 210,
		ASTC_8x6_LDR = 211,
		ASTC_8x8_LDR = 212,
		ASTC_10x5_LDR = 213,
		ASTC_10x6_LDR = 214,
		ASTC_10x8_LDR = 215,
		ASTC_10x10_LDR = 216,
		ASTC_12x10_LDR = 217,
		ASTC_12x12_LDR = 218,
		ASTC_4x4_HDR = 222,
		ASTC_5x4_HDR = 223,
		ASTC_5x5_HDR = 224,
		ASTC_6x5_HDR = 225,
		ASTC_6x6_HDR = 226,
		ASTC_8x5_HDR = 228,
		ASTC_8x6_HDR = 229,
		ASTC_8x8_HDR = 230,
		ASTC_10x5_HDR = 231,
		ASTC_10x6_HDR = 232,
		ASTC_10x8_HDR = 233,
		ASTC_10x10_HDR = 234,
		ASTC_12x10_HDR = 235,
		ASTC_12x12_HDR = 236,
		GBGR422 = 240,
		BGRG422 = 241,
		Depth16Unorm = 250,
		Depth32Float = 252,
		Stencil8 = 253,
		Depth24Unorm_Stencil8 = 255,
		Depth32Float_Stencil8 = 260,
		X32_Stencil8 = 261,
		X24_Stencil8 = 262,
		BGRA10_XR = 552,
		BGRA10_XR_sRGB = 553,
		BGR10_XR = 554,
		BGR10_XR_sRGB = 555
	};
	
	/// MARK: - Buffer
	class BLOOM_API BufferHandle: public ResourceHandle {
	public:
		BufferHandle() = default;
		BufferHandle(void* native, Deleter deleter, std::size_t size): ResourceHandle(native, deleter), _size(size) {}
		std::size_t size() const { return _size; }
		
	public:
		std::size_t _size = 0;
	};
	
	class BLOOM_API BufferView: public ResourceView {
	public:
		using ResourceView::ResourceView;
		BufferView(BufferHandle& buffer): ResourceView(buffer), _size(buffer.size()) {}
		std::size_t size() const { return _size; }
		
	public:
		std::size_t _size = 0;
	};
	
	///MARK: - Texture
	class BLOOM_API TextureHandle: public ResourceHandle {
	public:
		TextureHandle() = default;
		TextureHandle(void* native, Deleter deleter,
					  std::size_t width, std::size_t height, std::size_t depth):
			ResourceHandle(native, deleter),
			_width(width),
			_height(height),
			_depth(depth)
		{}
		
		std::size_t width() const { return _width; }
		std::size_t height() const { return _height; }
		std::size_t depth() const { return _depth; }
		
	private:
		std::size_t _width = 0;
		std::size_t _height = 0;
		std::size_t _depth = 0;
	};
	
	class BLOOM_API TextureView: public ResourceView {
	public:
		using ResourceView::ResourceView;
		TextureView(TextureHandle& texture):
			ResourceView(texture),
			_width(texture.width()),
			_height(texture.height()),
			_depth(texture.depth())
		{}
		
		std::size_t width() const { return _width; }
		std::size_t height() const { return _height; }
		std::size_t depth() const { return _depth; }
		
	private:
		std::size_t _width = 0;
		std::size_t _height = 0;
		std::size_t _depth = 0;
	};
	
	/// MARK: - RenderPipeline
	class BLOOM_API RenderPipelineHandle: public ResourceHandle {
	public:
		RenderPipelineHandle() = default;
		RenderPipelineHandle(void* native, Deleter deleter): ResourceHandle(native, deleter) {}
	};
	
	class BLOOM_API RenderPipelineView: public ResourceView {
	public:
		using ResourceView::ResourceView;
		RenderPipelineView(RenderPipelineHandle& pipeline):
			ResourceView(pipeline)
		{}
	};
	
	/// MARK: - DepthStencil
	class BLOOM_API DepthStencilHandle: public ResourceHandle {
	public:
		DepthStencilHandle() = default;
		DepthStencilHandle(void* native, Deleter deleter): ResourceHandle(native, deleter) {}
	};
	
	class BLOOM_API DepthStencilView: public ResourceView {
	public:
		using ResourceView::ResourceView;
		DepthStencilView(DepthStencilHandle& depthStencil):
			ResourceView(depthStencil)
		{}
	};
	
	/// MARK: - Sampler
	class BLOOM_API SamplerHandle: public ResourceHandle {
	public:
		SamplerHandle() = default;
		SamplerHandle(void* native, Deleter deleter): ResourceHandle(native, deleter) {}
	};
	
	class BLOOM_API SamplerView: public ResourceView {
	public:
		using ResourceView::ResourceView;
		SamplerView(SamplerHandle& depthStencil):
			ResourceView(depthStencil)
		{}
	};
	
}

