#pragma once

#include <variant>
#include <utl/functional.hpp>
#include <utl/vector.hpp>

#include <mtl/mtl.hpp>

namespace poppy {

	struct ToolbarButton {
		ToolbarButton(std::string label, utl::function<void()> block);
		
		std::string label;
		utl::function<void()> block;
	};
	
	struct ToolbarIconButton {
		ToolbarIconButton(char const* icon, utl::function<void()> block);
		ToolbarIconButton(utl::function<char const*()> icon,
						  utl::function<void()> block);
		ToolbarIconButton(utl::function<char const*()> icon,
						  utl::function<bool()> enabled,
						  utl::function<void()> block);
		
		ToolbarIconButton(utl::function<void()> block,
						  utl::function<char const*()> icon,
						  utl::function<char const*()> tooltip = nullptr,
						  utl::function<bool()> enabled = nullptr);
		
		utl::function<void()> block;
		utl::function<char const*()> icon;
		utl::function<char const*()> tooltip;
		utl::function<bool()> enabled; // treated as true if null
	};
	
	struct ToolbarSeparator {
		
	};
	
	struct ToolbarSpacer {
		
	};
	
	class ToolbarItem {
		friend class Toolbar;
	public:
		enum struct Type {
			button, iconButton, separator, spacer
		};
		
		ToolbarItem(ToolbarButton button):
			item(std::move(button))
		{}
		
		ToolbarItem(ToolbarIconButton button):
			item(std::move(button))
		{}
		
		ToolbarItem(ToolbarSeparator separator):
			item(std::move(separator))
		{}
		
		ToolbarItem(ToolbarSpacer spacer):
			item(std::move(spacer))
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
			ToolbarSeparator,
			ToolbarSpacer
		> item;
		float width = 0;
	};
	
    class Toolbar {
    public:
		Toolbar();
		Toolbar(std::initializer_list<ToolbarItem>);
		void addItem(ToolbarItem);
		
		float getWidthWithoutSpacers() const;
		
		void display(float width = 0.0f);
		
		float getHeight() const { return maxHeight; }
		void setHeight(float h) { maxHeight = h; }
		
	private:
		struct Block {
			float width = 0;
			float offset = 0;
			std::size_t itemBeginIndex = 0;
			std::size_t itemEndIndex = 0;
			bool visible;
		};
		
		void displayBlock(Block const&);
		void displayItem(ToolbarItem const&, std::size_t index);
		
		// called once after adding items
		void cook();
		// called every frame
		void calcOffsets(float totalWidth);
		
    private:
		utl::vector<ToolbarItem> items;
		utl::vector<Block> blocks;
		float maxHeight = 50;
		float height = 0;
		mtl::float2 position;
		bool cooked = false;
    };

}
