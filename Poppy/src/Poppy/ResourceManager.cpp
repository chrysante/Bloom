#include "ResourceManager.hpp"

#include "Bloom/Application/Resource.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include <stb/stb_image.h>

#include <Metal/Metal.hpp>
#include "Bloom/Platform/Metal/MetalRenderContext.hpp"

namespace poppy {
	
	template <typename T>
	T ResourceManager::getResource(std::string location) {
		auto const itr = resources.find(location);
		if (itr != resources.end()) {
			return std::get<T>(itr->second);
		}
		T const resource = loadResource<T>(location);
		resources.insert({ std::move(location), resource });
		return resource;
	}
	
	template <typename T>
	T ResourceManager::loadResource(std::string_view location) {
		if constexpr (std::is_same_v<T, TextureResource>) {
			using namespace bloom;
			auto const absolutePath = bloom::resourceDir() / location;
			int width, height, channels;
			auto* const imgData = stbi_load(absolutePath.c_str(), &width, &height, &channels, 4);
			
			auto texture = renderContext->createTexture({ width, height, 1 }, PixelFormat::RGBA8Unorm,
														TextureUsage::shaderRead, StorageMode::Private);
			
			/* copy image to texture */
			auto* const mrc = dynamic_cast<MetalRenderContext*>(renderContext);
			
			
			auto const sourceBuffer = mrc->createBuffer(imgData, width * height * channels, StorageMode::Managed);
			
			
			// Create a command buffer for GPU work.
			auto* const commandBuffer = mrc->commandQueue()->commandBuffer();


			// Encode a blit pass to copy data from the source buffer to the private texture.
			auto* const blitCommandEncoder = commandBuffer->blitCommandEncoder();
			
			blitCommandEncoder->copyFromBuffer((MTL::Buffer*)sourceBuffer.nativeHandle(),
											   0,
											   channels * width,
											   channels * width * height,
											   MTL::Size{ (uint64_t)width, (uint64_t)height, 1 },
											   (MTL::Texture*)texture.nativeHandle(),
											   0,
											   0,
											   MTL::Origin{});
			blitCommandEncoder->endEncoding();
			commandBuffer->commit();
			
			stbi_image_free(imgData);
			return TextureResource{ texture };
		}
		else {
			static_assert(utl::template_false<T>, "Not Implemented");
		}
	}

	template TextureResource ResourceManager::getResource(std::string);
	
}
