#include "Poppy/UI/Font.hpp"

#include <array>

#include <utl/format.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application.hpp"
#include "Poppy/Core/Debug.hpp"
#include "Poppy/UI/ImGuiContext.hpp"

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
///
///
FontMap poppy::fonts{};

void FontMap::init(bloom::Application& application) {
    static_cast<bloom::Emitter&>(*this) = application.makeEmitter();
    fonts.insert({ Font::UIDefault(), nullptr });
}

void FontMap::loadFonts(ImFontAtlas& atlas, float scaleFactor) {
    for (auto& [font, ptr]: fonts) {
        ptr = loadFont(font, atlas, scaleFactor);
    };

    //		for (auto const size: { FontSize::medium, FontSize::large }) {
    //			for (auto const weight: utl::enumerate<FontWeight>()) {
    //				for (auto const style: utl::enumerate<FontStyle>())
    //{ 					for (bool const monospaced: { false, true }) {
    //
    //
    //						if (monospaced && (int)weight <
    //(int)FontWeight::light) { 							continue;
    //						}
    //						if (monospaced && (int)weight >
    //(int)FontWeight::heavy) { 							continue;
    //						}
    //
    //						std::string const name =
    // utl::format("{}-{}-{}.ttf", 															 monospaced
    // ? "SFMono" : "SFPro", toString(weight), toString(style));
    //
    //						Font font{};
    //						font.size = size;
    //						font.weight = weight;
    //						font.style = style;
    //						font.monospaced = monospaced;
    //
    //						auto const path = bloom::resourceDir() / "Font" /
    // name;
    //
    //						auto imguiFontPtr =
    // atlas.AddFontFromFileTTF(path.c_str(),
    // (int)size
    // * scaleFactor, nullptr, nullptr); assert(imguiFontPtr);
    //
    //						auto const result = fonts.insert({ font,
    // imguiFontPtr }); 						assert(result.second);
    //					}
    //				}
    //			}
    //		}
}

ImFont* FontMap::loadFont(Font const& font,
                          ImFontAtlas& atlas,
                          float scaleFactor) {
    if (font.monospaced && (int)font.weight < (int)FontWeight::light) {
        return nullptr;
    }
    if (font.monospaced && (int)font.weight > (int)FontWeight::heavy) {
        return nullptr;
    }

    std::string const name = utl::format("{}-{}-{}.ttf",
                                         font.monospaced ? "SFMono" : "SFPro",
                                         toString(font.weight),
                                         toString(font.style));

    auto const path = bloom::resourceDir() / "Font" / name;

    auto* const imguiFontPtr =
        atlas.AddFontFromFileTTF(path.c_str(),
                                 (int)font.size * scaleFactor,
                                 nullptr,
                                 nullptr);
    assert(imguiFontPtr);
    return imguiFontPtr;
}

ImFont* FontMap::get(Font const& index) {
    assert((int)index.size > 0);
    auto const [itr, inserted] = fonts.insert({ index, nullptr });
    if (inserted) {
        dispatch(DispatchToken::nextFrame, ReloadFontAtlasCommand{});
    }
    return itr->second;

    //		ImFont* result = fonts[index];
    //		if (!result) {
    //
    //		}
    //		return result;
    //		auto const itr = fonts.find(font);
    //		if (itr == fonts.end()) {
    //			return nullptr;
    //		}
    //		return itr->second;
}
