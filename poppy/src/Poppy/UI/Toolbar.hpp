#pragma once

#include <variant>
#include <utl/functional.hpp>
#include <utl/vector.hpp>
#include <mtl/mtl.hpp>
#include <imgui/imgui.h>

namespace poppy {
	
	template <typename>
	class ToolbarItemBaseT;
	
	class ToolbarItemBase {
		template <typename>
		friend class ToolbarItemBaseT;
		friend class Toolbar;
		
	private:
		utl::function<char const*()> _tooltip;
		utl::function<bool()> _enabled; // treated as true if null
		ImGuiButtonFlags _addButtonFlags = 0;
	};

	template <typename Derived>
	class ToolbarItemBaseT: public ToolbarItemBase {
	protected:
		ToolbarItemBaseT() = default;
		
	public:
		Derived tooltip(char const* text) {
			this->_tooltip = [text]{ return text; };
			return asDerived();
		}
		
		Derived tooltip(utl::function<char const*()> text) {
			this->_tooltip = text;
			return asDerived();
		}
		
		Derived enabled(bool value) {
			this->_enabled = [value]{ return value; };
			return asDerived();
		}
		
		Derived enabled(utl::function<bool()> value) {
			this->_enabled = value;
			return asDerived();
		}
		
		Derived additionalButtonFlags(ImGuiButtonFlags flags) {
			this->_addButtonFlags = flags;
			return asDerived();
		}
		
	private:
		Derived const& asDerived() const {
			return *static_cast<Derived const*>(this);
		}
	};
	
	struct ToolbarButton: ToolbarItemBaseT<ToolbarButton> {
		ToolbarButton(std::string label):
			_label(std::move(label))
		{}

		ToolbarButton onClick(utl::function<void()> block) {
			this->_block = block;
			return *this;
		}
		
	private:
		friend class Toolbar;
		friend class ToolbarItemUnion;
		
		std::string _label;
		utl::function<void()> _block;
	};
	
	class ToolbarIconButton: public ToolbarItemBaseT<ToolbarIconButton> {
	public:
		ToolbarIconButton(char const* icon):
			_icon([icon]{ return icon; })
		{}
		
		ToolbarIconButton(utl::function<char const*()> icon):
			_icon(icon)
		{}

		ToolbarIconButton onClick(utl::function<void()> block) {
			this->_block = block;
			return *this;
		}
		
	private:
		friend class Toolbar;
		utl::function<void()> _block;
		utl::function<char const*()> _icon;
	};
	
	class ToolbarDropdownMenu: public ToolbarItemBaseT<ToolbarDropdownMenu> {
	public:
		ToolbarDropdownMenu();
		
		ToolbarDropdownMenu content(utl::function<void()> block) {
			this->_content = block;
			return *this;
		}
		
		ToolbarDropdownMenu previewValue(utl::function<std::string()> value) {
			_preview = value;
			return *this;
		}
		
		ToolbarDropdownMenu icon(utl::function<char const*()> icon) {
			_icon = icon;
			return *this;
		}
		
		ToolbarDropdownMenu icon(char const* icon) {
			_icon = [=]{ return icon; };
			return *this;
		}
		
		ToolbarDropdownMenu minSize(mtl::float2 size) {
			_minSize = size;
			return *this;
		}
		
		ToolbarDropdownMenu minWidth(float width) {
			_minSize.x = width;
			return *this;
		}
		
	private:
		friend class Toolbar;
		friend class ToolbarItemUnion;
		utl::function<void()> _content;
		utl::function<std::string()> _preview;
		utl::function<char const*()> _icon;
		std::string _id;
		mtl::float2 _minSize = 0;
	};
	
	struct ToolbarSeparator {
		
	};
	
	struct ToolbarSpacer {
		
	};
	
	struct CustomToolbarElement {
		CustomToolbarElement(utl::function<void(mtl::float2 size)> block):
			_block(std::move(block))
		{}
		CustomToolbarElement width(float width) {
			_width = width;
			return *this;
		}
		// CustomToolbarElement will behave like a spacer
		CustomToolbarElement useWidthAvail() {
			_width = 0;
			return *this;
		}
	private:
		friend class ToolbarItemUnion;
		friend class Toolbar;
		utl::function<void(mtl::float2 size)> _block;
		float _width = 0;
	};
		
	class ToolbarItemUnion {
		friend class Toolbar;
	public:
		enum struct Type {
			button, iconButton, dropdownMenu, separator, spacer, customElement
		};
		
		ToolbarItemUnion(ToolbarButton button):
			item(std::move(button))
		{}
		
		ToolbarItemUnion(ToolbarIconButton button):
			item(std::move(button))
		{}
		
		ToolbarItemUnion(ToolbarDropdownMenu menu):
			item(std::move(menu))
		{}
		
		ToolbarItemUnion(ToolbarSeparator separator):
			item(std::move(separator))
		{}
		
		ToolbarItemUnion(ToolbarSpacer spacer):
			item(std::move(spacer))
		{}
		
		ToolbarItemUnion(CustomToolbarElement element):
			item(std::move(element))
		{}
		
		Type type() const { return (Type)item.index(); }
		
	private:
		void calcWidth(float height);
		
		template <Type t>
		auto& get() { return utl::as_mutable(utl::as_const(*this).get<t>()); }
		template <Type t>
		auto const& get() const { return std::get<(std::size_t)t>(item); }
		
	private:
		std::variant<
			ToolbarButton,
			ToolbarIconButton,
			ToolbarDropdownMenu,
			ToolbarSeparator,
			ToolbarSpacer,
			CustomToolbarElement
		> item;
		float width = 0;
	};
	
	struct ToolbarStyle {
		float height = 40;
		float buttonAlpha = 0;
		float buttonAlphaHovered = 0.5;
		float buttonAlphaActive = 0.5;
	};
	
    class Toolbar {
    public:
		Toolbar();
		Toolbar(std::initializer_list<ToolbarItemUnion>);
		void addItem(ToolbarItemUnion);
		
		float getWidthWithoutSpacers() const;
		
		void display(float width = 0.0f);
		
		float getHeight() const { return style.height; }
		void setHeight(float value) { style.height = value; }
		
		ToolbarStyle getStyle() const { return style; }
		void setStyle(ToolbarStyle const& value) { style = value; }
		
	private:
		struct Block {
			float width = 0;
			float offset = 0;
			std::size_t itemBeginIndex = 0;
			std::size_t itemEndIndex = 0;
			bool visible;
		};
		
		void displayBlock(Block const&);
		void displayItem(ToolbarItemUnion const&, std::size_t index);
		
		bool button(char const* label,
					std::size_t id,
					mtl::float2 size,
					ImGuiButtonFlags addFlags,
					bool enabled = true) const;
		bool buttonEx(char const* label,
					  std::size_t id,
					  mtl::float2 size,
					  ImGuiButtonFlags addFlags,
					  bool enabled = true) const;
		bool iconButton(char const* icon,
						std::size_t id,
						mtl::float2 size,
						ImGuiButtonFlags addFlags,
						char const* tooltip = nullptr,
						bool enabled = true) const;
		bool beginCombo(ToolbarDropdownMenu const&, std::size_t id, mtl::float2 size) const;
		
		// called once after adding items
		void cook(bool full);
		// called every frame
		void calcOffsets(float totalWidth);
		
    private:
		utl::vector<ToolbarItemUnion> items;
		utl::vector<Block> blocks;
		ToolbarStyle style{};
		float actualHeight = 0;
		mtl::float2 position;
		bool cooked = false;
    };

}
