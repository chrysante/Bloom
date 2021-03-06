#pragma once

#include "Poppy/Core/Common.hpp"

#include <utl/hashmap.hpp>
#include <utl/vector.hpp>

#include <string>
#include <filesystem>
#include <imgui/imgui.h>

namespace poppy {
	
	enum class IconSize {
		_16 = 16, _24 = 24, _32 = 32, _48 = 48, _64 = 64
	};
	
	class IconFontMap {
	public:
		void load(ImFontAtlas& atlas, float scaleFactor,
				  std::filesystem::path config,
				  std::filesystem::path icons);
		
		WChar unicode(std::string key) const;
		std::array<char, 8> unicodeStr(std::string key) const;
	
		ImFont* font(IconSize) const;
		void addFont(IconSize, ImFont*);
		
		
		
	private:
		utl::hashmap<std::string, WChar> codes;
		utl::hashmap<IconSize, ImFont*> fonts;
		utl::vector<WChar> glyphs;
	};
	
	extern IconFontMap icons;
	
}
