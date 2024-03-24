#include "Poppy/UI/Font.h"

#include <array>

#include <utl/strcat.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/ImGuiContext.h"

using namespace bloom;
using namespace poppy;

std::string poppy::toString(FontWeight w) {
    return std::array{
        "Ultralight", "Thin", "Light", "Regular", "Medium",
        "Semibold",   "Bold", "Heavy", "Black"
    }[utl::to_underlying(w)];
}

std::string poppy::toString(FontStyle s) {
    return std::array{ "Roman", "Italic" }[utl::to_underlying(s)];
}

FontDesc FontDesc::UIDefault() {
    FontDesc font{};
    font.size = FontSize::Medium;
    font.weight = FontWeight::Regular;
    font.style = FontStyle::Roman;
    return font;
};

/// MARK: FontMap

FontMap poppy::fonts{};

void FontMap::init(bloom::Application& application) {
    static_cast<bloom::Emitter&>(*this) = application.makeEmitter();
    fonts.insert({ FontDesc::UIDefault(), nullptr });
}

void FontMap::loadFonts(ImFontAtlas& atlas, float scaleFactor) {
    for (auto& [font, ptr]: fonts) {
        ptr = loadFont(font, atlas, scaleFactor);
    }
}

static std::string toFontFileName(FontDesc const& font) {
    std::stringstream sstr;
    if (font.style == FontStyle::Monospaced) {
        sstr << "SFMono";
    }
    else {
        sstr << "SFPro";
    }
    sstr << "-" << toString(font.weight) << "-";
    if (font.style == FontStyle::Monospaced) {
        sstr << toString(FontStyle::Roman);
    }
    else {
        sstr << toString(font.style);
    }
    sstr << ".ttf";
    return std::move(sstr).str();
}

ImFont* FontMap::loadFont(FontDesc const& font, ImFontAtlas& atlas,
                          float scaleFactor) {
    if (font.style == FontStyle::Monospaced &&
        (int)font.weight < (int)FontWeight::Light)
    {
        return nullptr;
    }
    if (font.style == FontStyle::Monospaced &&
        (int)font.weight > (int)FontWeight::Heavy)
    {
        return nullptr;
    }
    auto path = bloom::resourceDir() / "Font" / toFontFileName(font);
    auto* imguiFontPtr =
        atlas.AddFontFromFileTTF(path.c_str(), (int)font.size * scaleFactor);
    assert(imguiFontPtr);
    return imguiFontPtr;
}

ImFont* FontMap::get(FontDesc const& index) {
    assert((int)index.size > 0);
    auto const [itr, inserted] = fonts.insert({ index, nullptr });
    if (inserted) {
        dispatch(DispatchToken::NextFrame, ReloadFontAtlasCommand{});
    }
    return itr->second;
}
