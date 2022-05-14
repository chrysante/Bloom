#include "IconConfig.hpp"

#include "Poppy/Debug.hpp"

#include "Bloom/Application/Resource.hpp"

#include <rapidjson/document.h>
#include <imgui/imgui_internal.h>
#include <fstream>
#include <sstream>

#include <utl/stdio.hpp>

namespace rapidjson {

	static auto begin(auto&& x) {
		return x.Begin();
	}

	static auto end(auto&& x) {
		return x.End();
	}

}

namespace poppy {
	
	void IconConfig::init(std::string text) {
		namespace rs = rapidjson;
		
		rs::Document doc;
		doc.Parse(text.data());
		
		for (auto&& i: doc["glyphs"]) {
			_instance._codes.insert({ i["css"].GetString(), i["code"].GetUint() });
		}
	}
	
	void IconConfig::initFromFile(std::filesystem::path path) {
		std::fstream file(bloom::pathForResource(path), std::ios::in);
		poppyExpect(!!file);
		std::stringstream sstr;
		sstr << file.rdbuf();
		
		init(sstr.str());
	}
	
	char16_t IconConfig::unicode(std::string name) {
		auto itr = _instance._codes.find(name);
		if (itr == _instance._codes.end()) {
			return 0;
		}
		return itr->second;
	}
	
	std::array<char, 8> IconConfig::unicodeStr(std::string name) {
		ImWchar const c = unicode(std::move(name));
		std::array<char, 8> result{};
		int count = ImTextStrToUtf8(result.data(), 8, &c, &c + 1);
		poppyAssert(count < 8);
		if (count == 0) {
			return { "???\0\0\0\0" };
		}
		return result;
	}
	
	void* IconConfig::font(int size) {
		auto itr = _instance._fonts.find(size);
		if (itr == _instance._fonts.end()) {
			return nullptr;
		}
		return itr->second;
	}

	void IconConfig::addFont(int size, void* font) {
		auto const result = _instance._fonts.insert({ size, font });
		poppyAssert(result.second);
	}
	
	utl::vector<std::uint16_t> IconConfig::glyphs() {
		utl::vector<std::uint16_t> result;
		result.reserve(_instance._codes.size());
		std::transform(_instance._codes.begin(),
					   _instance._codes.end(),
					   std::back_inserter(result),
					   [](auto&& p) {
			auto&& [_, code] = p;
			return static_cast<std::uint16_t>(code);
		});
		return result;
	}
	
	IconConfig IconConfig::_instance = {};
	
}
