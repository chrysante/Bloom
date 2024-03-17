#include "Poppy/UI/Icons.h"

#include <fstream>
#include <sstream>

#include <imgui_internal.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <utl/utility.hpp>

#include "Bloom/Application.h"
#include "Poppy/Core/Debug.h"

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
    codes.clear();
    glyphs.clear();
    fonts.clear();
    std::fstream file(configPath, std::ios::in);
    BL_ASSERT(file);
    rapidjson::IStreamWrapper stream(file);
    rapidjson::Document doc;
    doc.ParseStream(stream);
    for (auto const& glyph: doc["glyphs"]) {
        auto cssName = glyph["css"].GetString();
        auto code = glyph["code"].GetUint();
        if (code > USHRT_MAX) {
            continue;
        }
        codes.insert({ std::move(cssName), utl::narrow_cast<WChar>(code) });
    }
    glyphs.reserve(codes.size() + 1);
    std::transform(codes.begin(), codes.end(), std::back_inserter(glyphs),
                   [](auto const& p) { return p.second; });
    std::sort(glyphs.begin(), glyphs.end());
    glyphs.push_back(0); // Null terminator
    auto iconSizes = { IconSize::_16, IconSize::_24, IconSize::_32,
                       IconSize::_48 };
    for (auto size: iconSizes) {
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
