#include "ResourceUtil.hpp"

#include <algorithm>
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
        std::istreambuf_iterator<char> begin(file), end;
        utl::vector<char> buffer;
        std::copy(begin, end, std::back_inserter(buffer));
		return buffer;
	}
	
}
