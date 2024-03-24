#ifndef POPPY_UI_FONT_H
#define POPPY_UI_FONT_H

#include <filesystem>
#include <string>

#include <imgui.h>
#include <utl/hash.hpp>
#include <utl/hashmap.hpp>

#include "Bloom/Application/MessageSystem.h"

namespace bloom {

class Application;

} // namespace bloom

namespace poppy {

enum class FontSize { Medium = 17, Large = 34 };

enum class FontWeight {
    Ultralight,
    Thin,
    Light,
    Regular,
    Medium,
    Semibold,
    Bold,
    Heavy,
    Black,
    LAST = Black
};

enum class FontStyle { Roman, Italic, Monospaced, LAST = Monospaced };

/// Font attribute descriptor. Used to identify fonts when interacting with the
/// font manager
struct FontDesc {
    FontSize size;
    FontWeight weight;
    FontStyle style;

    /// \Returns the font descriptor used for most UI elements
    static FontDesc UIDefault();

    ///
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

    bool operator==(FontDesc const& rhs) const = default;
};

enum class IconSize { _16 = 16, _24 = 24, _32 = 32, _48 = 48, _64 = 64 };

struct IconFontDesc {
    IconSize size;

    bool operator==(IconFontDesc const& rhs) const = default;
};

} // namespace poppy

template <>
struct std::hash<poppy::FontDesc> {
    std::size_t operator()(poppy::FontDesc const& desc) const {
        return utl::hash_combine(desc.size, desc.weight, desc.style);
    }
};

template <>
struct std::hash<poppy::IconFontDesc> {
    std::size_t operator()(poppy::IconFontDesc const& desc) const {
        return std::hash<poppy::IconSize>{}(desc.size);
    }
};

namespace poppy {

///
class FontManager: public bloom::Emitter {
public:
    explicit FontManager(float scaleFactor);

    ~FontManager();

    ///
    static void setGlobalInstance(std::unique_ptr<FontManager> fontManager);

    ///
    static FontManager* getGlobalInstance();

    /// Tries to return the font described by \p desc
    /// If the font is not available, it will be loaded in the background. In
    /// this case another font is returned and the requested font will be
    /// returned by the next call to `get` as soon as the font is available
    static ImFont* get(FontDesc const& font);

    ///
    static ImFont* get(IconFontDesc const& font);

    ///
    static std::string getUnicodeStr(std::string name);

    /// # Temporary interface

    ImFontAtlas* getAtlas() { return imguiData.atlas; }

    using FontKey = std::variant<FontDesc, IconFontDesc>;
    struct ImGuiData {
        ImFontAtlas* atlas = nullptr;
        utl::hashmap<FontKey, ImFont*> map;
    };

private:
    ImFont* getImpl(FontKey const& key);

    std::string getUnicodeStrImpl(std::string name);

    void reloadAsync();

    void populateIcons();

    bool haveSignaledReload = false;
    float scaleFactor = 1;
    // TODO: Evaluate these members
    utl::hashmap<std::string, uint16_t> codes;
    std::vector<uint16_t> glyphs;
    ImGuiData imguiData;
};

struct ReloadedFontAtlasCommand {
    FontManager* fontManager;
};

} // namespace poppy

#endif // POPPY_UI_FONT_H
