#include "Poppy/UI/Font.h"

#include <array>
#include <fstream>

#include <imgui_internal.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <utl/strcat.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/ImGuiContext.h"

namespace rapidjson {

static auto begin(auto&& x) { return x.Begin(); }

static auto end(auto&& x) { return x.End(); }

} // namespace rapidjson

using namespace bloom;
using namespace poppy;

FontDesc FontDesc::UIDefault() {
    FontDesc font{};
    font.size = FontSize::Medium;
    font.weight = FontWeight::Regular;
    font.style = FontStyle::Roman;
    return font;
};

static std::string toString(FontWeight w) {
    return std::array{
        "Ultralight", "Thin", "Light", "Regular", "Medium",
        "Semibold",   "Bold", "Heavy", "Black"
    }[utl::to_underlying(w)];
}

static std::string toString(FontStyle s) {
    return std::array{ "Roman", "Italic" }[utl::to_underlying(s)];
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

void FontManager::init(bloom::Application& app) {
    static_cast<Emitter&>(*this) = app.makeEmitter();
    map.insert({ FontDesc::UIDefault(), nullptr });
}

static FontManager* gFontManager = nullptr;

void FontManager::setGlobalInstance(FontManager* fontManager) {
    gFontManager = fontManager;
}

FontManager* FontManager::getGlobalInstance() { return gFontManager; }

ImFont* FontManager::get(FontDesc const& desc) {
    return gFontManager->getImpl(desc);
}

ImFont* FontManager::get(IconFontDesc const& desc) {
    return gFontManager->getImpl(desc);
}

std::string FontManager::getUnicodeStr(std::string name) {
    return gFontManager->getUnicodeStrImpl(name);
}

ImFont* FontManager::getImpl(FontKey const& key) {
    auto [itr, inserted] = map.insert({ key, { nullptr } });
    if (inserted) {
        BL_ASSERT(std::holds_alternative<FontDesc>(key));
        dispatch(DispatchToken::NextFrame, ReloadFontAtlasCommand{});
        haveSignaledReload = true;
    }
    return itr->second;
}

std::string FontManager::getUnicodeStrImpl(std::string name) {
    auto itr = codes.find(name);
    if (itr == codes.end()) {
        return {};
    }
    uint16_t code = itr->second;
    std::array<char, 16> result{};
    int count = ImTextStrToUtf8(result.data(), 16, &code, &code + 1);
    BL_ASSERT(count < 16);
    if (count == 0) {
        return {};
    }
    return std::string(result.data());
}

void FontManager::reloadFonts(ImFontAtlas& atlas, float scaleFactor) {
    for (auto& [key, ptr]: map) {
        if (std::holds_alternative<FontDesc>(key)) {
            ptr = loadFont(std::get<FontDesc>(key), atlas, scaleFactor);
        }
    }
}

void FontManager::reloadIcons(ImFontAtlas& atlas, float scaleFactor,
                              std::filesystem::path configPath,
                              std::filesystem::path iconPath) {
    codes.clear();
    glyphs.clear();
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
        map[FontKey{ IconFontDesc{ size } }] = imguiFontPtr;
    }
}

ImFont* FontManager::loadFont(FontDesc const& font, ImFontAtlas& atlas,
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
