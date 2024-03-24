#ifndef POPPY_UI_FONT_H
#define POPPY_UI_FONT_H

#include <string>

#include <imgui.h>
#include <utl/hash.hpp>
#include <utl/hashmap.hpp>

#include "Bloom/Application/MessageSystem.h"

namespace bloom {

class Application;

} // namespace bloom

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

struct FontDesc {
    FontSize size;
    FontWeight weight;
    FontStyle style;
    bool monospaced;

    static FontDesc UIDefault() {
        FontDesc font{};

        font.size = FontSize::medium;
        font.weight = FontWeight::regular;
        font.style = FontStyle::roman;
        font.monospaced = false;

        return font;
    };

    [[nodiscard]] FontDesc setSize(FontSize size) const {
        auto result = *this;
        result.size = size;
        return result;
    }

    [[nodiscard]] FontDesc setWeight(FontWeight weight) const {
        auto result = *this;
        result.weight = weight;
        return result;
    }

    [[nodiscard]] FontDesc setStyle(FontStyle style) const {
        auto result = *this;
        result.style = style;
        return result;
    }

    [[nodiscard]] FontDesc setMonospaced(bool monospaced) const {
        auto result = *this;
        result.monospaced = monospaced;
        return result;
    }

    friend bool operator==(FontDesc const&, FontDesc const&) = default;
};

class FontMap: public bloom::Emitter {
public:
    void init(bloom::Application&);
    void loadFonts(ImFontAtlas&, float scaleFactor);
    ImFont* get(FontDesc const&);

private:
    static ImFont* loadFont(FontDesc const&, ImFontAtlas&, float scaleFactor);

private:
    utl::hashmap<FontDesc, ImFont*> fonts;
};

extern FontMap fonts;

} // namespace poppy

template <>
struct std::hash<poppy::FontDesc> {
    std::size_t operator()(poppy::FontDesc const& font) const {
        return utl::hash_combine(font.size, font.weight, font.style,
                                 font.monospaced);
    }
};

#endif // POPPY_UI_FONT_H
