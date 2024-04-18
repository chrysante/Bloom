#ifndef POPPY_UI_TOOLBAR_H
#define POPPY_UI_TOOLBAR_H

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <utl/common.hpp>
#include <vml/vml.hpp>
#ifndef __OBJC__
#include <range/v3/view.hpp>
#endif

#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Font.h"

namespace poppy {

///
enum class ToolbarFlags { None = 0, NoDecoration = 1 << 0 };

UTL_ENUM_OPERATORS(ToolbarFlags)

///
struct ToolbarStyle {
    float height = 30.0f;
    float buttonAlpha = 0.0f;
    float buttonAlphaHovered = 0.5f;
    float buttonAlphaActive = 0.5f;

    /// Padding. Defaults to `ImGui::GetStyle().WindowPadding`
    std::optional<vml::float2> padding;

    ///
    ToolbarFlags flags = ToolbarFlags::None;

    /// Computed by `Toolbar`
    float actualHeight;
};

/// Base class of all toolbar items
class ToolbarItem {
public:
    ToolbarItem() = default;
    ToolbarItem(ToolbarItem const&) = default;
    ToolbarItem(ToolbarItem&&) = default;
    virtual ~ToolbarItem() = default;

    /// \Returns a unique ID.
    /// This is simply the index of the item in the toolbar
    int ID() const { return _id; }

    ///
    float getWidth() const { return _width; }

private:
    friend class Toolbar;

    virtual void display(ToolbarStyle const& style) = 0;
    virtual float calcWidth(float height) const = 0;

    int _id;
    float _width;
};

/// Mixin class that provides common functionality of toolbar items
template <typename Derived>
class ToolbarItemCommon {
public:
    /// Specifies a tooltip that will be displayed when this item is hovered
    Derived& tooltip(std::string text) {
        this->_tooltip = [text] { return text.c_str(); };
        return asDerived();
    }

    /// \overload
    Derived& tooltip(std::function<char const*()> text) {
        this->_tooltip = text;
        return asDerived();
    }

    /// Specifies whether this item is disabled
    Derived& enabled(bool value) {
        this->_enabled = [value] { return value; };
        return asDerived();
    }

    /// \overload
    Derived& enabled(std::function<bool()> value) {
        this->_enabled = value;
        return asDerived();
    }

protected:
    char const* getTooltip() const { return _tooltip ? _tooltip() : nullptr; }
    bool getIsEnabled() const { return !_enabled || _enabled(); }

private:
    Derived& asDerived() { return *static_cast<Derived*>(this); }

    std::function<char const*()> _tooltip;
    std::function<bool()> _enabled; // treated as true if null
};

/// Common base class of toolbar buttons
class ToolbarButtonBase: public ToolbarItem {
protected:
    void doAction();

private:
    template <typename Derived>
    friend class ToolbarButtonMixin;
    friend class Toolbar;

    CommandPalette const* cmd = nullptr;
    std::function<void()> _action;
};

///
template <typename Derived>
class ToolbarButtonMixin {
public:
    /// Specify the action to perform when this button is clicked
    Derived& onClick(std::function<void()> action) {
        asDerived()._action = std::move(action);
        return asDerived();
    }

private:
    Derived& asDerived() { return *static_cast<Derived*>(this); }
};

/// Toolbar text button
class ToolbarButton:
    public ToolbarButtonBase,
    public ToolbarItemCommon<ToolbarButton>,
    public ToolbarButtonMixin<ToolbarButton> {
public:
    ToolbarButton(std::string label): _label(std::move(label)) {}

private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    std::string _label;
};

///
class ToolbarIconButton:
    public ToolbarButtonBase,
    public ToolbarItemCommon<ToolbarIconButton>,
    public ToolbarButtonMixin<ToolbarIconButton> {
public:
    /// Construct from an icon name
    ToolbarIconButton(std::string iconName):
        _icon([=] { return iconName.c_str(); }) {}

    /// Construct from a function that returns an icon name
    ToolbarIconButton(std::function<char const*()> iconNameFn):
        _icon(iconNameFn) {}

    /// Construct from an action and an icon name
    ToolbarIconButton(CommandPalette::Action const* action,
                      std::string iconName);

private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    std::function<char const*()> _icon;
};

/// A toolbar button that opens a dropdown menu
class ToolbarDropdownMenu:
    public ToolbarItem,
    public ToolbarItemCommon<ToolbarDropdownMenu> {
public:
    ToolbarDropdownMenu();

    /// Specifies the content callback
    ToolbarDropdownMenu& content(std::function<void()> content) {
        this->_content = std::move(content);
        return *this;
    }

    /// The preview value should be the active option of the dropdown menu. If
    /// this is not set a down arrow will be used instead
    ToolbarDropdownMenu& previewValue(std::function<std::string()> value) {
        _preview = value;
        return *this;
    }

    ///
    ToolbarDropdownMenu& icon(std::function<char const*()> iconNameFn) {
        _icon = iconNameFn;
        return *this;
    }

    ///
    ToolbarDropdownMenu& icon(char const* iconName) {
        return icon([=] { return iconName; });
    }

    ///
    ToolbarDropdownMenu& minSize(vml::float2 size) {
        _minSize = size;
        return *this;
    }

    ///
    ToolbarDropdownMenu& minWidth(float width) {
        _minSize.x = width;
        return *this;
    }

protected:
    bool beginCombo(ToolbarStyle const& style);

    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    std::function<void()> _content;
    std::function<std::string()> _preview;
    std::function<char const*()> _icon;
    std::string _id;
    vml::float2 _minSize = 0;
};

///
class ToolbarSeparator: public ToolbarItem {
private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;
};

/// Spacers are empty items that take up as much space as possible. They can be
/// used to split a toolbar into multiple sections
class ToolbarSpacer: public ToolbarItem {
private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;
};

/// Non-interactive text item
class ToolbarTextItem: public ToolbarItem {
public:
    /// Sets the displayed text
    ToolbarTextItem& text(std::string text) {
        return this->text([=] { return text.c_str(); });
    }

    /// \overload
    ToolbarTextItem& text(std::function<char const*()> textFn) {
        _text = std::move(textFn);
        return *this;
    }

    /// Sets the font used to render the text
    ToolbarTextItem& font(FontDesc fontDesc) {
        return font([=] { return fontDesc; });
    }

    /// \overload
    ToolbarTextItem& font(std::function<FontDesc()> fontDescFn) {
        _fontDesc = std::move(fontDescFn);
        return *this;
    }

private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    char const* getText() const;
    FontDesc getFontDesc() const;

    std::function<char const*()> _text;
    std::function<FontDesc()> _fontDesc;
};

/// Empty item with fixed size
class ToolbarEmptyItem: public ToolbarItem {
public:
    ToolbarEmptyItem& width(float width) {
        _width = width;
        return *this;
    }

private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    float _width = 0;
};

///
class ToolbarItemList {
#ifndef __OBJC__
    static auto getImpl(auto* This) {
        return This->_items |
               ranges::views::transform([](auto& p) -> auto& { return *p; });
    }

    static auto getCommon(auto* This) {
        return getImpl(This) | ranges::views::common;
    }
#endif

public:
    template <std::derived_from<ToolbarItem>... ItemType>
    ToolbarItemList(ItemType... items) {
        _items.reserve(sizeof...(items));
        (add(std::move(items)), ...);
    }

    ///
    template <std::derived_from<ToolbarItem> ItemType>
    void add(ItemType item) {
        _items.push_back(std::make_unique<ItemType>(std::move(item)));
    }

    /// Range interface @{
#ifndef __OBJC__
    auto begin() { return getCommon(this).begin(); }
    auto begin() const { return getCommon(this).begin(); }
    auto end() { return getCommon(this).end(); }
    auto end() const { return getCommon(this).end(); }
#endif
    size_t size() const { return _items.size(); }
    size_t empty() const { return _items.empty(); }
    auto& operator[](size_t index) { return *_items[index]; }
    auto& operator[](size_t index) const { return *_items[index]; }
    /// @}

private:
    std::vector<std::unique_ptr<ToolbarItem>> _items;
};

///
class Toolbar {
public:
    Toolbar(CommandPalette const* cmd = nullptr, ToolbarItemList items = {});

    /// \Returns the acccumulated width of all items including padding but
    /// ignoring all spacers. This is used by the dockspace for the triple
    /// toolbar layout
    float getWidthWithoutSpacers() const;

    /// Displays the toolbar. Will fill available width unless parameter \p
    /// width is provided
    void display(std::optional<float> width = std::nullopt) noexcept;

    /// \Returns the actual height of the toolbar including padding
    float getHeight() const;

    void setHeight(float value) { style.height = value; }

    ///
    ToolbarStyle const& getStyle() const { return style; }

    ///
    void setStyle(ToolbarStyle const& value) { style = value; }

private:
    /// A block is a list of items without spacers
    struct Block {
        float width = 0;
        float offset = 0;
        size_t itemBeginIndex = 0;
        size_t itemEndIndex = 0;
        bool visible;
    };

    void displayBlock(Block const&);

    /// Must be called after adding items
    void cook();

    /// Called every frame
    void calcOffsets(float totalWidth);

    vml::float2 getPadding() const;

    ToolbarItemList items;
    std::vector<Block> blocks;
    ToolbarStyle style{};
};

} // namespace poppy

#endif // POPPY_UI_TOOLBAR_H
