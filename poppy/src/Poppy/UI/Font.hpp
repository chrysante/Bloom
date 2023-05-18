#pragma once

#include <string>

#include <imgui/imgui.h>
#include <utl/hash.hpp>
#include <utl/hashmap.hpp>

#include "Bloom/Application/MessageSystem.hpp"

namespace bloom {

class Application;

}

namespace poppy {

enum class FontSize { _16 = 17, _32 = 34, medium = _16, large = _32 };

enum class FontWeight {
    ultralight = 0,
    thin,
    light,
    regular,
    medium,
    semibold,
    bold,
    heavy,
    black,
    _count
};

std::string toString(FontWeight);

enum class FontStyle { roman = 0, italic = 1, _count };

std::string toString(FontStyle);

struct Font {
    FontSize size;
    FontWeight weight;
    FontStyle style;
    bool monospaced;

    static Font UIDefault() {
        Font font{};

        font.size       = FontSize::medium;
        font.weight     = FontWeight::regular;
        font.style      = FontStyle::roman;
        font.monospaced = false;

        return font;
    };

    Font setSize(FontSize size) const {
        auto result = *this;
        result.size = size;
        return result;
    }
    Font setWeight(FontWeight weight) const {
        auto result   = *this;
        result.weight = weight;
        return result;
    }
    Font setStyle(FontStyle style) const {
        auto result  = *this;
        result.style = style;
        return result;
    }
    Font setMonospaced(bool monospaced) const {
        auto result       = *this;
        result.monospaced = monospaced;
        return result;
    }

    friend bool operator==(Font const&, Font const&) = default;
};

class FontMap: public bloom::Emitter {
public:
    void init(bloom::Application&);
    void loadFonts(ImFontAtlas&, float scaleFactor);
    ImFont* get(Font const&);

private:
    static ImFont* loadFont(Font const&, ImFontAtlas&, float scaleFactor);

private:
    utl::hashmap<Font, ImFont*> fonts;
};

extern FontMap fonts;

} // namespace poppy

template <>
struct std::hash<poppy::Font> {
    std::size_t operator()(poppy::Font const& font) const {
        return utl::hash_combine(font.size,
                                 font.weight,
                                 font.style,
                                 font.monospaced);
    }
};
