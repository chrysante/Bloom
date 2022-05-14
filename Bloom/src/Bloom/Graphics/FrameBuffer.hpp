#pragma once

#include "Bloom/Core/Base.hpp"
#include "RenderPrimitives.hpp"

#include <mtl/mtl.hpp>

namespace bloom {
	
	class RenderContext;
	
	struct BLOOM_API FrameBuffer {
		FrameBuffer() = default;
		
		static FrameBuffer create(RenderContext*, std::size_t width, std::size_t height);
		mtl::usize2 size() const { return { color.width(), color.height() }; }
		
		TextureHandle color;
		TextureHandle depth;
		TextureHandle finalImage;
	};
	
	struct BLOOM_API EditorFrameBuffer: public FrameBuffer {
		EditorFrameBuffer(FrameBuffer rhs): FrameBuffer(std::move(rhs)) {}
		
		EditorFrameBuffer() = default;
		
		static EditorFrameBuffer create(RenderContext*, std::size_t width, std::size_t height);
		
		TextureHandle editorDepth;
		TextureHandle entityID;
		TextureHandle shadowCascade;
		TextureHandle selected;
		TextureHandle finalImageEditor;
	};
	
}
