#ifndef POPPY_UI_FONT_H
#define POPPY_UI_FONT_H

#include <filesystem>
#include <string>
#include <string_view>

#include <imgui.h>
#include <utl/hash.hpp>
#include <utl/hashmap.hpp>
#include <utl/overload.hpp>

#include "Bloom/Application/MessageSystem.h"

namespace bloom {

class Application;

} // namespace bloom

namespace poppy {

enum class FontSize { Medium = 17, Default = Medium, Large = 34 };

template <typename T>
    requires std::is_arithmetic_v<T>
FontSize operator+(FontSize s, T t) {
    return FontSize{ (T)s + t };
}

template <typename T>
    requires std::is_arithmetic_v<T>
FontSize operator+(T t, FontSize s) {
    return FontSize{ t + (T)s };
}

template <typename T>
    requires std::is_arithmetic_v<T>
FontSize operator-(FontSize s, T t) {
    return FontSize{ (T)s - t };
}

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

template <typename T>
concept FontParameterType = std::is_same_v<T, FontSize> ||
                            std::is_same_v<T, FontWeight> ||
                            std::is_same_v<T, FontStyle>;

/// Font attribute descriptor. Used to identify fonts when interacting with the
/// font manager
struct FontDesc {
    FontSize size = FontSize::Medium;
    FontWeight weight = FontWeight::Regular;
    FontStyle style = FontStyle::Roman;

    /// \Returns the font descriptor used for most UI elements
    static FontDesc UIDefault();

    /// \overload
    static FontDesc UIDefault(FontParameterType auto... args) {
        return UIDefault().apply(args...);
    }

    ///
    static FontDesc MonoDefault();

    ///
    FontDesc& setSize(FontSize size) {
        this->size = size;
        return *this;
    }

    FontDesc& setWeight(FontWeight weight) {
        this->weight = weight;
        return *this;
    }

    FontDesc& setStyle(FontStyle style) {
        this->style = style;
        return *this;
    }

    FontDesc& apply(FontParameterType auto... args) {
        utl::overload apply = {
            [&](FontSize size) { setSize(size); },
            [&](FontWeight weight) { setWeight(weight); },
            [&](FontStyle style) { setStyle(style); },
        };
        (apply(args), ...);
        return *this;
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

/// Result structure of `FontManager::get(IconFontDesc desc, std::string name)`
struct IconData {
    /// The font that encodes the icon
    ImFont* font;

    /// The string that represents the icon
    std::string utf8Rep;
};

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

    /// Convenience overload
    static ImFont* get(FontParameterType auto... params) {
        return get(FontDesc::UIDefault(params...));
    }

    ///
    static IconData get(IconFontDesc const& font, std::string iconName);

    ///  \Returns the ImGui font atlas
    ImFontAtlas* getAtlas() { return imguiData.atlas; }

    struct Impl;

private:
    using FontKey = std::variant<FontDesc, IconFontDesc>;

    struct ImGuiData {
        ImFontAtlas* atlas = nullptr;
        utl::hashmap<FontKey, ImFont*> map;
    };

    ImFont* getImpl(FontKey const& key);

    std::string getUnicodeStrImpl(IconSize size, std::string_view name);

    void reloadAsync();
    void startReloadThread();
    void completeReloadingSync(ImGuiData data);

    /// Maps CSS icon names to unicode representation. This lists all icons
    /// available in the resource files
    utl::hashmap<std::string, uint16_t> allIconCodes;
    utl::hashmap<IconSize, utl::hashset<uint16_t>> glyphMap;
    ImGuiData imguiData;
    float scaleFactor = 1;
    bool haveSignaledReload = false;
    bool isReloading = false;
};

struct ReloadedFontAtlasCommand {
    FontManager* fontManager;
};

} // namespace poppy

#endif // POPPY_UI_FONT_H
