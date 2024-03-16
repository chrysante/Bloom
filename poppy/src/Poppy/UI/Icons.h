#ifndef POPPY_UI_ICONS_H
#define POPPY_UI_ICONS_H

#include <filesystem>
#include <string>
#include <vector>

#include <imgui.h>
#include <utl/hashmap.hpp>

#include "Poppy/Core/Common.h"

namespace poppy {

enum class IconSize { _16 = 16, _24 = 24, _32 = 32, _48 = 48, _64 = 64 };

class IconFontMap {
public:
    void load(ImFontAtlas& atlas, float scaleFactor,
              std::filesystem::path config, std::filesystem::path icons);

    WChar unicode(std::string key) const;
    std::array<char, 8> unicodeStr(std::string key) const;

    ImFont* font(IconSize) const;
    void addFont(IconSize, ImFont*);

private:
    utl::hashmap<std::string, WChar> codes;
    utl::hashmap<IconSize, ImFont*> fonts;
    std::vector<WChar> glyphs;
};

extern IconFontMap icons;

} // namespace poppy

#endif // POPPY_UI_ICONS_H
