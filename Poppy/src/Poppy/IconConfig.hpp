#pragma once

#include <utl/hashmap.hpp>
#include <utl/vector.hpp>

#include <string>
#include <filesystem>

namespace poppy {
	
	class IconConfig {
	public:
		static void init(std::string);
		static void initFromFile(std::filesystem::path);
		static char16_t unicode(std::string);
		static std::array<char, 8> unicodeStr(std::string);
		static void* font(int);
		
		static void addFont(int, void*);
		
		static utl::vector<std::uint16_t> glyphs();
		
	private:
		static IconConfig _instance;
		utl::hashmap<std::string, char16_t> _codes;
		utl::hashmap<int, void*> _fonts;
	};
	
}
