#pragma once

#include "FilesystemHistory.hpp"

#include "Poppy/Core/Common.hpp"
#include "Poppy/UI/Toolbar.hpp"

#include "Bloom/Core/Serialize.hpp"
#include "Bloom/Application/MessageSystem.hpp"

#include <utl/functional.hpp>
#include <utl/vector.hpp>
#include <utl/utility.hpp>
#include <mtl/mtl.hpp>
#include <optional>
#include <filesystem>
#include <span>

namespace poppy {
	
	constexpr std::string_view FileDragDropType = "PoppyFileDragDrop";
	
	struct DirectoryItemDescription {
		std::string label;
		std::string iconName;
		void const* displayImage = nullptr;
		utl::function<void()> dragDropSource;
	};
	
	class DirectoryViewDelegate {
	public:
		virtual DirectoryItemDescription makeItemDescription(std::filesystem::directory_entry const&) const;
	};

	struct DirectoryViewDisplayDescription {
		bool noToolbar = false;
	};
	
	class DirectoryView: public bloom::Emitter {
	public:
		enum class SortCondition {
			type, name, _count
		};
		
	public:
		DirectoryView();
		
		void display(DirectoryViewDisplayDescription const& = {});
		
		/// MARK: Queries
		std::filesystem::path currentDirectory() const;
		std::filesystem::path rootDirectory() const;
		bool canGoUp() const;
		bool canGoBack() const;
		bool canGoForward() const;
		SortCondition sortCondition() const { return desc.sortCondition; }
		
		/// MARK: Modifiers
		YAML::Node serialize() const;
		void deserialize(YAML::Node const&);
		void rescan();
		void openDirectory(std::filesystem::path);
		void setRootDirectory(std::filesystem::path);
		void goUp();
		void goBack();
		void goForward();
		void setSortCondition(SortCondition);
		void setDelegate(std::unique_ptr<DirectoryViewDelegate> delegate) { this->delegate = std::move(delegate); }
		void setDelegate(DirectoryViewDelegate* delegate) { setDelegate(std::unique_ptr<DirectoryViewDelegate>(delegate)); }
		
	public:
		struct Description {
			SortCondition sortCondition = SortCondition::type;
			bool group = false;
		};
		
		/// MARK: Private
		struct Entry: std::filesystem::directory_entry {
			mtl::rectangle<float> bb;
			bool selected = false;
			std::optional<bool> selectionCandidate;
			char renaming = false;
			
			bool displaySelected() const;
		};
		
	private:
		struct Directory: public utl::vector<Entry> {
			std::filesystem::path path;
			std::size_t activeIndex = (std::size_t)-1;
		};
		
		struct ItemDisplayResult {
			bool hovered = false;
			bool held = false;
			bool selected = false;
			bool activated = false;
			bool activatedByDragDrop = false;
		};
		
		void addressbar(mtl::float2 size);
		void drawAddressbarButton(std::size_t index, mtl::float2 position, mtl::float2 size, uint32_t color, std::string_view label) const;
		void addressbarPopup(bool open, std::size_t directoryLineIndex, mtl::float2 position);
		bool addressbarPopupMenuItem(std::filesystem::directory_entry const&);
		
		std::size_t displayGroup(Directory&,
								 std::size_t beginIndex,
								 void (DirectoryView::*)(std::span<Entry>, std::size_t));
		
		void itemPopup(std::size_t index, Entry&, DirectoryItemDescription const&);
		void itemRenameField(std::size_t index, Entry&, mtl::float2 position, float width);
		void itemDragSource(Entry const&, std::invocable<Entry> auto&& drawFn);
		void itemDragTarget(Entry const&);
		utl::small_vector<char> makeDragdropPayloadFromSelection() const;
		utl::small_vector<std::filesystem::path> unloadDragdropPayload(char const* data, std::size_t size) const;
		
		/// MARK: TableView
		void displayTableView();
		void displayTable(std::span<Entry>, std::size_t beginIndex = 0);
		ItemDisplayResult displayTableViewItem(float width,
											   std::size_t index,
											   Entry&);
		void drawTableViewLabelAndIcon(mtl::float2 position,
									   mtl::float2 size,
									   mtl::float2 sizeNoLabel,
									   std::string_view label,
									   std::string_view icon,
									   void const* image,
									   bool renaming = false) const;
		
		/// MARK: Background
		void displayBackground();
		void backgroundContextMenu();
		void beginDragSelection();
		void updateDragSelection();
		
		/// MARK: Logic
		bool setDirectory(std::filesystem::path);
		void scanDirectory(Directory&, auto const& selectionBackup) const;
		void scanDirectoryLine(bool preserveSelection = true);
		void renameEntry(Entry&, std::string newName);
		void renameEntry(Entry const&, std::string newName);
		void moveEntry(Entry, std::filesystem::path to);
		void deleteEntry(Entry);
		void newFolder();
		void gotoNextFrame(std::filesystem::path const&);
		
		/// MARK: Selection
		void clearSelection();
		void select(std::size_t index);
		void selectEx(auto&&, bool clear = true);
		void updateSelectionRect(mtl::rectangle<float> const&);
		void applySelectionRect(mtl::rectangle<float> const&);
		
		///
		auto const& currDir() const { return directoryLine.back(); }
		auto& currDir() { return utl::as_mutable(utl::as_const(*this).currDir()); }
		
		
		auto const& rootDir() const { return directoryLine.front(); }
		auto& rootDir() { return utl::as_mutable(utl::as_const(*this).rootDir()); }
		
	private:
		Description desc;
		std::unique_ptr<DirectoryViewDelegate> delegate;
		utl::vector<Directory> directoryLine;
		std::filesystem::path rootDirPath;
		
		Toolbar toolbar;
		FilesystemHistory history;
		
		/// Selection
		std::optional<mtl::float2> selectionDragBegin;
		
		std::array<char, 256> renameBuffer{};
	};

}


BLOOM_MAKE_TEXT_SERIALIZER(poppy::DirectoryView::Description,
						   sortCondition,
						   group);
