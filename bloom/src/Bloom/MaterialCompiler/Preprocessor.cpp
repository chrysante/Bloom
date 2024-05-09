#include "Bloom/MaterialCompiler/Preprocessor.h"

#include <cassert>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <utl/strcat.hpp>

using namespace bloom;

namespace {

struct Preprocessor {
    std::string source;
    std::span<std::filesystem::path const> searchPaths;
    void run();
    std::string openFile(std::filesystem::path name) const;

    size_t index = 0;
};

} // namespace

std::string bloom::preprocessIncludes(
    std::string source, std::span<std::filesystem::path const> searchPaths) {
    Preprocessor pp{ std::move(source), searchPaths };
    pp.run();
    return std::move(pp).source;
}

void Preprocessor::run() {
    static std::regex const r(
        R"__([ \t]*#[ \t]*include[ \t]*"([^"]*)"[ \t]*)__");
    while (true) {
        std::sregex_iterator begin(source.begin() + index, source.end(), r);
        std::sregex_iterator end;
        if (begin == end) {
            return;
        }
        std::smatch match = *begin;
        auto filename = match.str(1);
        std::string contents = openFile(filename);
        source.replace(index + match.position(), match.str(0).size(), contents);
        index += match.position() + contents.size();
    }
}

std::string Preprocessor::openFile(std::filesystem::path name) const {
    auto completePath = [&]() -> std::filesystem::path {
        if (std::filesystem::exists(name)) {
            return name;
        }
        for (auto& searchPath: searchPaths) {
            auto complete = searchPath / name;
            if (std::filesystem::exists(complete)) {
                return complete;
            }
        }
        throw std::runtime_error(utl::strcat("Failed to open file ", name));
    }();
    std::fstream file(completePath);
    assert(file);
    std::stringstream sstr;
    sstr << file.rdbuf();
    std::string contents = std::move(sstr).str();
    return preprocessIncludes(contents, searchPaths);
}
