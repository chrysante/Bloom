#pragma once

#include "Panel.hpp"
#include <filesystem>

namespace poppy {
	
	class AssetBrowser: public Panel {
	public:
		AssetBrowser();
		
	private:
		void init() override;
		void display() override;
		
		void toolbar();
		
	private:
		std::filesystem::path workingDir;
		float itemSize = 60;
	};
	
}
