#include "ResourceUtil.hpp"

#include <fstream>
#include <sstream>

namespace bloom {
	
	BLOOM_API std::optional<std::string> readFileText(std::filesystem::path const& path) {
		std::fstream file(path, std::ios::in);
		if (!file) {
			return std::nullopt;
		}
		std::stringstream sstr;
		sstr << file.rdbuf();
		return sstr.str();
	}
	
	BLOOM_API std::optional<utl::vector<char>> readFileBinary(std::filesystem::path const& path) {
		std::fstream file(path, std::ios::in | std::ios::binary);
		if (!file) {
			return std::nullopt;
		}
		utl::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
		return buffer;
	}
	
	
}
