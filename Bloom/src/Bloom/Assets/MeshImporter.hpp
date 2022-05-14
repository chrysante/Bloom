#pragma once

#include "Bloom/Assets/ConcreteAssets.hpp"
#include <filesystem>

namespace bloom {
	
	class MeshImporter {
	public:
		StaticMeshData import(std::filesystem::path);
		
	private:
		
	};
	
}
