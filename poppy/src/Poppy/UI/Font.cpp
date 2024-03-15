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

/// MARK: FontMap

FontMap poppy::fonts{};

void FontMap::init(bloom::Application& application) {
    static_cast<bloom::Emitter&>(*this) = application.makeEmitter();
    fonts.insert({ Font::UIDefault(), nullptr });
}

void FontMap::loadFonts(ImFontAtlas& atlas, float scaleFactor) {
    for (auto& [font, ptr]: fonts) {
        ptr = loadFont(font, atlas, scaleFactor);
    }
}

ImFont* FontMap::loadFont(Font const& font, ImFontAtlas& atlas,
                          float scaleFactor) {
    if (font.monospaced && (int)font.weight < (int)FontWeight::light) {
        return nullptr;
    }
    if (font.monospaced && (int)font.weight > (int)FontWeight::heavy) {
        return nullptr;
    }
    std::string const name = utl::strcat(font.monospaced ? "SFMono" : "SFPro",
                                         "-", toString(font.weight), "-",
                                         toString(font.style), ".ttf");
    auto const path = bloom::resourceDir() / "Font" / name;
    auto* const imguiFontPtr =
        atlas.AddFontFromFileTTF(path.c_str(), (int)font.size * scaleFactor,
                                 nullptr, nullptr);
    assert(imguiFontPtr);
    return imguiFontPtr;
}

ImFont* FontMap::get(Font const& index) {
    assert((int)index.size > 0);
    auto const [itr, inserted] = fonts.insert({ index, nullptr });
    if (inserted) {
        dispatch(DispatchToken::NextFrame, ReloadFontAtlasCommand{});
    }
    return itr->second;
}
