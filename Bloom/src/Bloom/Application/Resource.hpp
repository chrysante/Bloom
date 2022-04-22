#pragma once

#include "Bloom/Core/Base.hpp"

#include <filesystem>
#include <utl/functional.hpp>

namespace bloom {
	
	BLOOM_API void* loadTextureFromFile(void* device, std::filesystem::path);
	
	BLOOM_API std::filesystem::path currentDirectoryPath();
	BLOOM_API std::filesystem::path executablePath();
	
	BLOOM_API std::filesystem::path pathForResource(std::filesystem::path);
	BLOOM_API std::filesystem::path resourceDir();
	BLOOM_API std::filesystem::path getLibraryDir();
	
	BLOOM_API void showSaveFilePanel(utl::function<void(std::string)> completion);
	BLOOM_API void showOpenFilePanel(utl::function<void(std::string)> completion);
	BLOOM_API void showSelectDirectoryPanel(utl::function<void(std::string)> completion);
	
}

