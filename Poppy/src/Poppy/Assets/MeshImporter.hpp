#pragma once

#include "Bloom/Assets/StaticMeshAsset.hpp"

namespace poppy {
	
	class MeshImporter {
	public:
		bloom::StaticMeshData import(std::filesystem::path);
		
	private:
		
	};
	
}
