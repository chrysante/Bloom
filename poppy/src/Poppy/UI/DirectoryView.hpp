#pragma once

#include "FilesystemHistory.hpp"

#include "Poppy/Core/Common.hpp"
#include "Poppy/UI/Toolbar.hpp"
#include "Poppy/UI/Font.hpp"

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
		
		enum class Layout {
			columns, table, list, _count
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
			Layout layout = Layout::table;
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
		
		template <bool IsConst>
		class EntryHandleEx {
			using DirectoryType = std::conditional_t<IsConst, Directory const, Directory>;
			using EntryType = std::conditional_t<IsConst, Entry const, Entry>;
			template <bool> friend class EntryHandleEx;
			
		public:
			EntryHandleEx() = default;
			EntryHandleEx(DirectoryType& directory, std::size_t index):
				mDirectory(&directory), mIndex(index)
			{}
			EntryHandleEx(EntryHandleEx<false> rhs):
				mDirectory(rhs.mDirectory), mIndex(rhs.mIndex)
			{}
			
			DirectoryType& directory() const { return *mDirectory; }
			std::size_t index() const { return mIndex; }
			EntryType& get() const { return directory()[index()]; }
			EntryType* operator->() const { return &get(); }
			
		private:
			DirectoryType* mDirectory = nullptr;
			std::size_t mIndex = 0;
		};
		
		using EntryHandle = EntryHandleEx<false>;
		using ConstEntryHandle = EntryHandleEx<true>;
		
		struct ItemButtonState {
			bool hovered             : 1 = false;
			bool held                : 1 = false;
			bool selected            : 1 = false;
			bool activated           : 1 = false;
			bool activatedByDragDrop : 1 = false;
		};
		
		struct SelectionDragState {
			mtl::float2 beginPosition = 0;
			std::size_t directoryIndex = -1;
		};
		
		/// MARK: Addressbar
		void addressbar(mtl::float2 size);
		void drawAddressbarButton(std::size_t index, mtl::float2 position, mtl::float2 size, uint32_t color, std::string_view label) const;
		void addressbarPopup(bool open, std::size_t directoryLineIndex, mtl::float2 position);
		bool addressbarPopupMenuItem(std::filesystem::directory_entry const&);
		
		/// MARK: Item View Generic
		std::size_t displayGroup(Directory&,
								 std::size_t beginIndex,
								 utl::function<void(std::size_t, std::size_t)> cb);
		ItemButtonState displayItem(mtl::rectangle<float> const& bb,
									mtl::rectangle<float> const& labelBB,
									mtl::rectangle<float> const& iconBB,
									TextAlign,
									EntryHandle,
									int addFrameDrawFlags = 0);
		void drawItem(mtl::rectangle<float> const& bb,
					  mtl::rectangle<float> const& labelBB,
					  mtl::rectangle<float> const& iconBB,
					  TextAlign,
					  std::string_view label,
					  std::string_view icon,
					  void const* image,
					  bool renaming = false) const;
		void drawItemFrame(mtl::rectangle<float> const& bb,
						   std::uint32_t color,
						   int addDrawFlags = 0) const;
		void itemPopup(EntryHandle, DirectoryItemDescription const&);
		void itemRenameField(std::size_t index, EntryHandle, mtl::float2 position, float width);
		void handleItemButtonState(ItemButtonState, EntryHandle);
		void itemDragSource(ConstEntryHandle, std::invocable<Entry, mtl::float2> auto&& drawFn);
		void itemDragTarget(Directory&);
		void itemDragTarget(ConstEntryHandle);
		void itemDragTargetEx(std::filesystem::path const&);
		utl::small_vector<char> makeDragdropPayloadFromSelection(Directory const&) const;
		utl::small_vector<std::filesystem::path> unloadDragdropPayload(char const* data, std::size_t size) const;
		utl::small_vector<std::string_view> peekDragdropPayload(char const* data, std::size_t size) const;
		
		/// MARK: Column View
		void displayColumnView();
		void displayColumn(Directory&, std::size_t directoryIndex);
		void displayPreviewColumn();
		ItemButtonState displayColumnViewItem(EntryHandle);
		
		/// MARK: Table View
		void displayTableView();
		void displayTable(Directory&, std::size_t beginIndex, std::size_t endIndex);
		ItemButtonState displayTableViewItem(float width, EntryHandle);
		
		/// MARK: Background
		void displayBackground(Directory&);
		void backgroundContextMenu(Directory&);
		void beginDragSelection(Directory&);
		void updateDragSelection(Directory&);
		
		/// MARK: Logic
		bool setDirectory(std::filesystem::path);
		void scanDirectory(Directory&, auto const& selectionBackup) const;
		void scanDirectoryLine(std::filesystem::path leafDirectory, bool preserveSelection = true);
		void renameEntry(ConstEntryHandle, std::string newName);
		void renameEntry(EntryHandle, std::string newName);
		void moveElement(std::filesystem::path element, std::filesystem::path to);
		void deleteElement(std::filesystem::path element);
		void newFolder(Directory&);
		void gotoNextFrame(std::filesystem::path const&);
		
		/// MARK: Selection
		void clearSelection(Directory&);
		void select(EntryHandle);
		void selectEx(Directory&, auto&&, bool clear = true);
		void updateSelectionRect(Directory&, mtl::rectangle<float> const&);
		void applySelectionRect(Directory&, mtl::rectangle<float> const&);
		
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
		std::optional<SelectionDragState> selectionDragState;
		
		std::array<char, 256> renameBuffer{};
	};

}


BLOOM_MAKE_TEXT_SERIALIZER(poppy::DirectoryView::Description,
						   sortCondition,
						   layout,
						   group);
