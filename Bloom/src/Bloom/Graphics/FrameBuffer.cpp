#include "FrameBuffer.hpp"

#include "RenderContext.hpp"

namespace bloom {
	
	FrameBuffer FrameBuffer::create(RenderContext* renderContext,
									std::size_t width,
									std::size_t height)
	{
		FrameBuffer result;
		result.color = renderContext->createRenderTarget(width, height,
														 PixelFormat::RGBA32Float,
														 StorageMode::Private);
		result.depth = renderContext->createRenderTarget(width, height,
														 PixelFormat::Depth32Float,
														 StorageMode::Private);
		result.finalImage = renderContext->createRenderTarget(width, height,
															  PixelFormat::RGBA8Unorm,
															  StorageMode::Private);
		return result;
	}
	
	EditorFrameBuffer EditorFrameBuffer::create(RenderContext* renderContext,
												std::size_t width,
												std::size_t height)
	{
		EditorFrameBuffer result(FrameBuffer::create(renderContext, width, height));
		
		result.editorDepth = renderContext->createRenderTarget(width, height,
															   PixelFormat::Depth32Float,
															   StorageMode::Private);
		result.entityID = renderContext->createRenderTarget(width, height,
															PixelFormat::R32Uint,
															StorageMode::Managed);
		result.shadowCascade = renderContext->createRenderTarget(width, height,
																 PixelFormat::RGBA8Unorm,
																 StorageMode::Private);
		result.selected = renderContext->createRenderTarget(width, height,
															PixelFormat::R8Unorm,
															StorageMode::Private);
		result.finalImageEditor = renderContext->createRenderTarget(width, height,
																	PixelFormat::RGBA8Unorm,
																	StorageMode::Private);
		
		return result;
	}
	
}
