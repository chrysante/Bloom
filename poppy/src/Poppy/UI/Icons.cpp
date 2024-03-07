#include "Poppy/UI/Icons.hpp"

#include <fstream>
#include <sstream>

#include <imgui/imgui_internal.h>
#include <rapidjson/document.h>
#include <utl/stdio.hpp>

#include "Bloom/Application.hpp"
#include "Poppy/Core/Debug.hpp"

namespace rapidjson {

static auto begin(auto&& x) { return x.Begin(); }

static auto end(auto&& x) { return x.End(); }

} // namespace rapidjson

using namespace bloom;

namespace poppy {

IconFontMap icons{};

void IconFontMap::load(ImFontAtlas& atlas, float scaleFactor,
                       std::filesystem::path configPath,
                       std::filesystem::path iconPath) {
    std::fstream file(configPath, std::ios::in);
    assert(!!file);
    std::stringstream sstr;
    sstr << file.rdbuf();
    std::string const configText = sstr.str();
    namespace rs = rapidjson;

    rs::Document doc;
    doc.Parse(configText.data());

    codes.clear();

    for (auto&& i: doc["glyphs"]) {
        codes.insert({ i["css"].GetString(), i["code"].GetUint() });
    }

    glyphs.reserve(codes.size() + 1);
    std::transform(codes.begin(), codes.end(), std::back_inserter(glyphs),
                   [](auto&& p) {
        auto&& [_, code] = p;
        return code;
    });
    glyphs.push_back(0); // null terminator

    auto const iconSizes = { IconSize::_16, IconSize::_24, IconSize::_32,
                             IconSize::_48 };

    fonts.clear();

    for (auto const size: iconSizes) {
        auto imguiFontPtr = atlas.AddFontFromFileTTF(iconPath.c_str(),
                                                     (int)size * scaleFactor,
                                                     nullptr, glyphs.data());

        addFont(size, imguiFontPtr);
    }
}

WChar IconFontMap::unicode(std::string name) const {
    auto itr = codes.find(name);
    if (itr == codes.end()) {
        return 0;
    }
    return itr->second;
}

std::array<char, 8> IconFontMap::unicodeStr(std::string name) const {
    ImWchar const c = unicode(std::move(name));
    std::array<char, 8> result{};
    int count = ImTextStrToUtf8(result.data(), 8, &c, &c + 1);
    assert(count < 8);
    if (count == 0) {
        return { "?\0\0\0\0\0\0" };
    }
    return result;
}

ImFont* IconFontMap::font(IconSize size) const {
    auto const itr = fonts.find(size);
    if (itr == fonts.end()) {
        return nullptr;
    }
    return itr->second;
}

void IconFontMap::addFont(IconSize size, ImFont* font) {
    auto const result = fonts.insert({ size, font });
    assert(result.second);
}

} // namespace poppy
