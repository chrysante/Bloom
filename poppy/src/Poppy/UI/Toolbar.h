#ifndef POPPY_UI_TOOLBAR_H
#define POPPY_UI_TOOLBAR_H

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <utl/common.hpp>
#include <vml/vml.hpp>

#include "Poppy/UI/Font.h"
#include "Poppy/UI/Fwd.h"

namespace poppy {

///
enum class ToolbarFlags { None = 0, NoDecoration = 1 << 0 };

UTL_ENUM_OPERATORS(ToolbarFlags)

///
struct ToolbarStyle {
    float height = 26.0f;
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

    /// \Returns `true` is this item is enabled. Disabled items are not
    /// displayed
    bool isEnabled() const { return doIsEnabled(); }

    ///  \Returns an optional tooltip string for this item
    char const* getTooltip() const { return doGetTooltip(); }

private:
    friend class Toolbar;

    virtual void display(ToolbarStyle const& style) = 0;
    virtual float calcWidth(float height) const = 0;
    virtual bool doIsEnabled() const { return true; }
    virtual char const* doGetTooltip() const { return nullptr; }

    int _id;
    float _width;
};

/// Mixin class that provides common functionality of toolbar items
template <typename Derived>
class ToolbarItemMixin: public ToolbarItem {
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
    Derived& asDerived() { return *static_cast<Derived*>(this); }

private:
    bool doIsEnabled() const override { return !_enabled || _enabled(); }
    char const* doGetTooltip() const override {
        return _tooltip ? _tooltip() : nullptr;
    }

    std::function<char const*()> _tooltip;
    std::function<bool()> _enabled;
};

/// Common base class of toolbar buttons
template <typename Derived>
class ToolbarButtonMixin: public ToolbarItemMixin<Derived> {
public:
    /// Specify the action to perform when this button is clicked
    Derived& onClick(std::function<void()> action) {
        _action = std::move(action);
        return this->asDerived();
    }

protected:
    void doAction() const;

private:
    std::function<void()> _action;
};

/// Toolbar text button
class ToolbarButton: public ToolbarButtonMixin<ToolbarButton> {
public:
    ToolbarButton(std::string label): _label(std::move(label)) {}

private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    std::string _label;
};

///
class ToolbarIconButton: public ToolbarButtonMixin<ToolbarIconButton> {
public:
    /// Construct from an icon name
    ToolbarIconButton(std::string iconName):
        _icon([=] { return iconName.c_str(); }) {}

    /// Construct from a function that returns an icon name
    ToolbarIconButton(std::function<char const*()> iconNameFn):
        _icon(iconNameFn) {}

    /// Construct from an command and an icon name
    ToolbarIconButton(Command const* command, std::string iconName);

private:
    void display(ToolbarStyle const& style) override;
    float calcWidth(float height) const override;

    std::function<char const*()> _icon;
};

/// A toolbar button that opens a dropdown menu
class ToolbarDropdownMenu: public ToolbarItemMixin<ToolbarDropdownMenu> {
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
class Toolbar {
public:
    template <std::derived_from<ToolbarItem>... ItemTypes>
    Toolbar(ItemTypes... items) {
        (addItem(std::move(items)), ...);
    }

    /// \Returns the acccumulated width of all items including padding but
    /// ignoring all spacers. This is used by the dockspace for the triple
    /// toolbar layout
    float getWidthWithoutSpacers() const;

    /// Displays the toolbar. Will fill available width unless parameter \p
    /// width is provided
    void display(std::optional<float> width = std::nullopt) noexcept;

    /// \Returns the actual height of the toolbar including padding
    float getHeight() const;

    ///
    void setHeight(float value) { style.height = value; }

    ///
    ToolbarStyle const& getStyle() const { return style; }

    ///
    void setStyle(ToolbarStyle const& value) { style = value; }

    /// \Returns the resolved padding value
    vml::float2 getPadding() const;

    /// Overrides the padding
    void setPadding(std::optional<vml::float2> value) { style.padding = value; }

private:
    /// A block is a list of items without spacers
    struct Block {
        float width = 0;
        float offset = 0;
        size_t itemBeginIndex = 0;
        size_t itemEndIndex = 0;
        bool visible;
    };

    template <typename Item>
    void addItem(Item&& item) {
        allItems.push_back(std::make_unique<std::remove_cvref_t<Item>>(
            std::forward<Item>(item)));
    }

    void displayBlock(Block const&);

    /// Must be called after adding items
    void cook();

    /// Called every frame
    void calcOffsets(float totalWidth);

    std::vector<std::unique_ptr<ToolbarItem>> allItems;
    std::vector<ToolbarItem*> activeItems;
    std::vector<Block> blocks;
    ToolbarStyle style{};
};

} // namespace poppy

#endif // POPPY_UI_TOOLBAR_H
