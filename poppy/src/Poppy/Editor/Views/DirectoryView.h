#ifndef POPPY_EDITOR_VIEWS_DIRECTORYVIEW_H
#define POPPY_EDITOR_VIEWS_DIRECTORYVIEW_H

#include <any>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include <mtl/mtl.hpp>
#include <utl/vector.hpp>

namespace poppy {

/// Passed to directory view callbacks
struct DirectoryEntry {
    std::filesystem::path path;
    std::any userData;
};

/// Set of callbacks to control the behaviour of a `DirectoryView`
struct DirectoryViewDelegate {
    /// Action performed when a file is double clicked
    std::function<void(DirectoryEntry const&)> openFile;

    /// Action performed when a folder is double clicked. If this is null the
    /// folder will be opened in this view
    std::function<void(DirectoryEntry const&)> openFolder;

    /// Shall return a CSS icon name for the given file. This will change in the
    /// future when we have proper thumbnails. If this is null a default icon
    /// will be used
    std::function<std::string(DirectoryEntry const&)> selectIcon;

    /// Shall return `true` if the given file or directory shall be displayed in
    /// the view. If this is null all files are displayed
    std::function<bool(DirectoryEntry const&)> shallDisplay;

    /// Used to create user data that will be associated with every directory
    /// entry and passed with it to the other callbacks
    std::function<std::any(std::filesystem::path const&)> makeUserData;

    /// Use this callback to draw ImGui menu item into the popup menu for the
    /// given file
    std::function<void(DirectoryEntry const&)> popupMenu;

    /// Use this callback to call `ImGui::SetDragDropPayload`
    std::function<void(DirectoryEntry const&)> dragdropPayload;
};

/// General purpose file directory viewer, meant to be used as a subview of a
/// `View`-derived class
class DirectoryView {
public:
    explicit DirectoryView(DirectoryViewDelegate delegate = {});

    /// To be called every frame for rendering and interaction
    void display();

    /// Opens \p directory in this viewer
    void openDirectory(std::filesystem::path const& directory);

    /// Assigns the delegate
    void setDelegate(DirectoryViewDelegate delegate) {
        this->delegate = delegate;
    }

private:
    /// Used for drawing the entries
    struct Cursor {
        mtl::float2 position = 0;
        int index = 0;
    };

    enum class EntryType { Directory, File };

    /// Represents one entry in a directory, i.e. a file or a folder
    struct EntryEx: DirectoryEntry {
        EntryType type;
        std::string label;
        std::string icon;
    };

    struct Parameters {
        mtl::float2 itemSpacing = 10;
        mtl::float2 itemSize = 100;
        float labelHeight = 20;
    };

    /// Displays and handles interaction of one entry
    void displayEntry(Cursor const& cursor, EntryEx const& entry);

    ///
    void advanceCursor(Cursor& cursor, bool forceLineBreak = false);

    ///
    EntryEx makeEntryEx(DirectoryEntry publicEntry,
                        std::filesystem::directory_entry const& fsEntry) const;

    /// Delegate wrappers
    /// @{
    void handleOpen(EntryEx const& entry);
    bool shallDisplay(DirectoryEntry const& entry) const;
    std::string selectIconName(DirectoryEntry const& entry) const;
    std::any makeUserData(std::filesystem::directory_entry const& entry) const;
    /// @}

    ///
    DirectoryViewDelegate delegate;

    /// The entries in the current directory
    std::vector<EntryEx> entries;

    Parameters params;
    int renaming = -1;
    bool setRenameFocus = false;
    std::array<char, 256> renameBuffer{};
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_DIRECTORYVIEW_H
