#include "Poppy/UI/Font.h"

#include <array>
#include <fstream>
#include <future>
#include <span>

#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
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

struct FontManager::Impl {
    using FontKey = FontManager::FontKey;
    using ImGuiData = FontManager::ImGuiData;
};

using FontKey = FontManager::Impl::FontKey;
using ImGuiData = FontManager::Impl::ImGuiData;

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

static void loadIcons(
    ImGuiData& data, float scaleFactor,
    utl::hashmap<IconSize, std::vector<uint16_t>> const& sortedGlyphMap) {
    std::filesystem::path iconPath = resourceDir() / "Icons/Icons.ttf";
    for (auto& [size, glyphs]: sortedGlyphMap) {
        ImFont* imguiFontPtr =
            data.atlas->AddFontFromFileTTF(iconPath.c_str(),
                                           (int)size * scaleFactor, nullptr,
                                           glyphs.data());
        data.map[FontKey{ IconFontDesc{ size } }] = imguiFontPtr;
    }
}

static utl::hashmap<std::string, uint16_t> loadAllIconCodes() {
    auto resourcePath = resourceDir() / "Icons/IconsConfig.json";
    std::fstream file(resourcePath, std::ios::in);
    if (!file) {
        Logger::Error("Failed to load icon resource file: ", resourcePath, " ",
                      strerror(errno));
        return {};
    }
    rapidjson::IStreamWrapper stream(file);
    rapidjson::Document doc;
    doc.ParseStream(stream);
    utl::hashmap<std::string, uint16_t> codes;
    for (auto const& glyph: doc["glyphs"]) {
        auto cssName = glyph["css"].GetString();
        auto code = glyph["code"].GetUint();
        if (code > USHRT_MAX) {
            Logger::Warn("Skipping icon (unicode values exeeds range): ",
                         cssName);
            continue;
        }
        codes.insert({ std::move(cssName), utl::narrow_cast<WChar>(code) });
    }
    return codes;
}

static void loadAll(
    ImGuiData& data, float scaleFactor,
    utl::hashmap<IconSize, std::vector<uint16_t>> const& sortedGlyphMap) {
    if (!data.atlas) {
        data.atlas = IM_NEW(ImFontAtlas);
    }
    data.atlas->Clear();
    loadFonts(data, scaleFactor);
    loadIcons(data, scaleFactor, sortedGlyphMap);
    data.atlas->Build();
}

FontDesc FontDesc::UIDefault() {
    FontDesc font{};
    font.size = FontSize::Medium;
    font.weight = FontWeight::Regular;
    font.style = FontStyle::Roman;
    return font;
}

FontDesc FontDesc::MonoDefault() {
    FontDesc font{};
    font.size = FontSize{ 14 };
    font.weight = FontWeight::Regular;
    font.style = FontStyle::Monospaced;
    return font;
}

FontManager::FontManager(float scaleFactor):
    allIconCodes(loadAllIconCodes()),
    imguiData{ .atlas = IM_NEW(ImFontAtlas) },
    scaleFactor(scaleFactor) {
    imguiData.map.insert({ FontDesc::UIDefault(), nullptr });
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

IconData FontManager::get(IconFontDesc const& desc, std::string name) {
    return { gFontManager->getImpl(desc),
             gFontManager->getUnicodeStrImpl(desc.size, std::move(name)) };
}

static utl::hashmap<IconSize, std::vector<uint16_t>> makeSortedGlyphMap(
    utl::hashmap<IconSize, utl::hashset<uint16_t>> const& iconGlyphs) {
    utl::hashmap<IconSize, std::vector<uint16_t>> result;
    for (auto& [key, set]: iconGlyphs) {
        auto& glyphs = result[key];
        glyphs = set.values();
        ranges::sort(glyphs);
        glyphs.push_back(0); // Null terminator
    }
    return result;
}

ImFont* FontManager::getImpl(FontKey const& key) {
    auto [itr, inserted] = imguiData.map.insert({ key, { nullptr } });
    if (inserted) {
        reloadAsync();
    }
    return itr->second;
}

static std::string unicodeToUTF8(uint16_t code) {
    std::array<char, 16> result{};
    int count = ImTextStrToUtf8(result.data(), 16, &code, &code + 1);
    BL_ASSERT(count < 16);
    if (count == 0) {
        return {};
    }
    return std::string(result.data());
}

std::string FontManager::getUnicodeStrImpl(IconSize size,
                                           std::string_view name) {
    auto itr = allIconCodes.find(name);
    if (itr == allIconCodes.end()) {
        return {};
    }
    auto code = itr->second;
    if (glyphMap[size].insert(code).second) {
        reloadAsync();
    }
    return unicodeToUTF8(code);
}

void FontManager::reloadAsync() {
    if (haveSignaledReload) {
        return;
    }
    haveSignaledReload = true;
    /// We start the async operation next frame because when a new view opens
    /// that uses multiple unavailable fonts, we want to gather all fonts we
    /// need to load and then start the reload operation
    dispatch(DispatchToken::NextFrame, [this] { startReloadThread(); });
}

void FontManager::startReloadThread() {
    if (isReloading) {
        return;
    }
    haveSignaledReload = false;
    isReloading = true;
    std::thread([this, scaleFactor = scaleFactor,
                 sortedGlyphMap = makeSortedGlyphMap(glyphMap),
                 data = imguiData]() mutable {
        BL_PROFILE_FUNCTION("Font Reloading");
        data.atlas = IM_NEW(ImFontAtlas);
        loadAll(data, scaleFactor, sortedGlyphMap);
        postEmptySystemEvent();
        dispatch(DispatchToken::NextFrame, [this, data = std::move(data)] {
            completeReloadingSync(std::move(data));
        });
    }).detach();
}

static void amendMap(utl::hashmap<FontKey, ImFont*>& dest,
                     utl::hashmap<FontKey, ImFont*> const& source) {
    dest.insert(source.begin(), source.end());
}

void FontManager::completeReloadingSync(ImGuiData data) {
    isReloading = false;
    IM_FREE(imguiData.atlas);
    imguiData.atlas = data.atlas;
    /// We amend the new map with all entries that were added during the built
    amendMap(data.map, imguiData.map);
    imguiData.map = std::move(data.map);
    dispatch(DispatchToken::Now, ReloadedFontAtlasCommand{ this });
    if (haveSignaledReload) {
        startReloadThread();
    }
}
