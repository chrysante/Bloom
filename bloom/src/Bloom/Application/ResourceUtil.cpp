#include "Bloom/Application/ResourceUtil.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace bloom;

static void printFileError(std::string_view path) {
    std::cerr << "Failed to open file: " << path << ": " << strerror(errno)
              << std::endl;
}

BLOOM_API std::optional<std::string> bloom::readFileText(
    std::filesystem::path const& path) {
    std::fstream file(path, std::ios::in);
    if (!file) {
        printFileError(path.string());
        return std::nullopt;
    }
    std::stringstream sstr;
    sstr << file.rdbuf();
    return sstr.str();
}

BLOOM_API std::optional<utl::vector<char>> bloom::readFileBinary(
    std::filesystem::path const& path) {
    std::fstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        printFileError(path.string());
        return std::nullopt;
    }
    std::istreambuf_iterator<char> begin(file), end;
    utl::vector<char> buffer;
    std::copy(begin, end, std::back_inserter(buffer));
    return buffer;
}
