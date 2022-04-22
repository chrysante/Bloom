#pragma once

#include "Bloom/Core/Base.hpp"
#include "RenderPrimitives.hpp"

#include <mtl/mtl.hpp>

namespace bloom {
	
	class RenderContext;
	
	class BLOOM_API FrameBuffer {
	public:
		FrameBuffer() = default;
		
		
		TextureView color() { return _colorBuffer; }
		TextureView depth() { return _depthBuffer; }
		TextureView finalImage() { return _finalImageBuffer; }
		
		mtl::usize2 size() const { return { _colorBuffer.width(), _colorBuffer.height() }; }
		
		static FrameBuffer create(RenderContext*, std::size_t width, std::size_t height);
		
	private:
		TextureHandle _colorBuffer;
		TextureHandle _depthBuffer;
		TextureHandle _finalImageBuffer;
	};
	
	class BLOOM_API EditorFrameBuffer: public FrameBuffer {
		EditorFrameBuffer(FrameBuffer rhs): FrameBuffer(std::move(rhs)) {}
		
		friend class Renderer;
		
	public:
		EditorFrameBuffer() = default;
		TextureView entityID() { return _entityIDBuffer; }
		TextureView selected() { return _selectedBuffer; }
		
		TextureView finalImageEditor() { return _finalImageEditorBuffer; }
		
		static EditorFrameBuffer create(RenderContext*, std::size_t width, std::size_t height);
		
	private:
		TextureHandle _entityIDBuffer;
		TextureHandle _selectedBuffer;
		TextureHandle _finalImageEditorBuffer;
	};
	
}
