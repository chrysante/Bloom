#pragma once

#include "Bloom/Core/Core.hpp"

namespace bloom {
	
	struct BLOOM_API BloomFramebuffer {
		static constexpr int numDSMipLevels = 8;
		static constexpr int numUSMipLevels = numDSMipLevels - 1;

		TextureHandle downsample;
		std::array<TextureHandle, numDSMipLevels> downsampleMips;
		TextureHandle upsample;
		std::array<TextureHandle, numUSMipLevels> upsampleMips;
	};
	
}
