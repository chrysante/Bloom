#include "Poppy/UI/Font.h"

#include <array>
#include <fstream>
#include <future>
#include <span>

#include <imgui_internal.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <utl/strcat.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiContext.h"

namespace rapidjson {

static auto begin(auto&& x) { return x.Begin(); }

static auto end(auto&& x) { return x.End(); }

} // namespace rapidjson

using namespace bloom;
using namespace poppy;

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

using FontKey = FontManager::FontKey;

using ImGuiData = FontManager::ImGuiData;

static ImFont* loadFont(FontDesc const& font, ImFontAtlas& atlas,
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

static void loadFonts(ImGuiData& data, float scaleFactor) {
    for (auto& [key, ptr]: data.map) {
        if (std::holds_alternative<FontDesc>(key)) {
            ptr = loadFont(std::get<FontDesc>(key), *data.atlas, scaleFactor);
        }
    }
}

static void loadIcons(ImGuiData& data, float scaleFactor,
                      std::span<uint16_t const> glyphs) {
    std::filesystem::path iconPath = resourceDir() / "Icons/Icons.ttf";
    auto iconSizes = { IconSize::_16, IconSize::_24, IconSize::_32,
                       IconSize::_48 };
    for (auto size: iconSizes) {
        auto imguiFontPtr =
            data.atlas->AddFontFromFileTTF(iconPath.c_str(),
                                           (int)size * scaleFactor, nullptr,
                                           glyphs.data());
        data.map[FontKey{ IconFontDesc{ size } }] = imguiFontPtr;
    }
}

static void loadAll(ImGuiData& data, float scaleFactor,
                    std::vector<uint16_t> glyphs) {
    if (!data.atlas) {
        data.atlas = IM_NEW(ImFontAtlas);
    }
    data.atlas->Clear();
    loadFonts(data, scaleFactor);
    loadIcons(data, scaleFactor, glyphs);
    data.atlas->Build();
}

FontDesc FontDesc::UIDefault() {
    FontDesc font{};
    font.size = FontSize::Medium;
    font.weight = FontWeight::Regular;
    font.style = FontStyle::Roman;
    return font;
};

FontManager::FontManager(float scaleFactor):
    scaleFactor(scaleFactor), imguiData{ .atlas = IM_NEW(ImFontAtlas) } {
    imguiData.map.insert({ FontDesc::UIDefault(), nullptr });
    populateIcons();
    loadAll(imguiData, scaleFactor, glyphs);
}

FontManager::~FontManager() {
    if (imguiData.atlas) {
        IM_FREE(imguiData.atlas);
    }
}

static std::unique_ptr<FontManager> gFontManager = nullptr;

void FontManager::setGlobalInstance(std::unique_ptr<FontManager> fontManager) {
    gFontManager = std::move(fontManager);
}

FontManager* FontManager::getGlobalInstance() { return gFontManager.get(); }

ImFont* FontManager::get(FontDesc const& desc) {
    return gFontManager->getImpl(desc);
}

ImFont* FontManager::get(IconFontDesc const& desc) {
    return gFontManager->getImpl(desc);
}

std::string FontManager::getUnicodeStr(std::string name) {
    return gFontManager->getUnicodeStrImpl(name);
}

void FontManager::populateIcons() {
    std::filesystem::path configPath = resourceDir() / "Icons/IconsConfig.json";
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
}

ImFont* FontManager::getImpl(FontKey const& key) {
    auto [itr, inserted] = imguiData.map.insert({ key, { nullptr } });
    if (inserted) {
        BL_ASSERT(std::holds_alternative<FontDesc>(key));
        reloadAsync();
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

void FontManager::reloadAsync() {
    if (haveSignaledReload) {
        return;
    }
    haveSignaledReload = true;
    /// We start the async operation next frame because when a new view opens
    /// that uses multiple unavailable fonts, we want to gather all fonts we
    /// need to load and then start the reload operation
    dispatch(DispatchToken::NextFrame, [this] {
        haveSignaledReload = false;
        Logger::Info("Reloading Fonts");
        std::thread([this, scaleFactor = scaleFactor, glyphs = glyphs,
                     data = imguiData]() mutable {
            data.atlas = IM_NEW(ImFontAtlas);
            loadAll(data, scaleFactor, glyphs);
            postEmptySystemEvent();
            dispatch(DispatchToken::NextFrame, [this, data = std::move(data)] {
                IM_FREE(imguiData.atlas);
                imguiData = std::move(data);
                dispatch(DispatchToken::Now, ReloadedFontAtlasCommand{ this });
                Logger::Info("Done Reloading Fonts");
            });
        }).detach();
    });
}
