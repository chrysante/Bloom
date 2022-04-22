#include "FrameBuffer.hpp"

#include "RenderContext.hpp"

namespace bloom {
	
	FrameBuffer FrameBuffer::create(RenderContext* renderContext,
									std::size_t width,
									std::size_t height)
	{
		FrameBuffer result;
		result._colorBuffer = renderContext->createRenderTarget(width, height,
																PixelFormat::RGBA32Float,
																StorageMode::Private);
		result._depthBuffer = renderContext->createRenderTarget(width, height,
																PixelFormat::Depth32Float,
																StorageMode::Private);
		result._finalImageBuffer = renderContext->createRenderTarget(width, height,
																	 PixelFormat::RGBA8Unorm,
																	 StorageMode::Private);
		return result;
	}
	
	EditorFrameBuffer EditorFrameBuffer::create(RenderContext* renderContext,
												std::size_t width,
												std::size_t height)
	{
		EditorFrameBuffer result(FrameBuffer::create(renderContext, width, height));
		result._entityIDBuffer = renderContext->createRenderTarget(width, height,
																   PixelFormat::R32Uint,
																   StorageMode::Managed);
		result._selectedBuffer = renderContext->createRenderTarget(width, height,
																   PixelFormat::R8Unorm,
																   StorageMode::Private);
		result._finalImageEditorBuffer = renderContext->createRenderTarget(width, height,
																		   PixelFormat::RGBA8Unorm,
																		   StorageMode::Private);
		return result;
	}
	
}
