#ifndef POPPY_VIEWS_DIRECTORYVIEW_H
#define POPPY_VIEWS_DIRECTORYVIEW_H

#include "Bloom/Application/MessageSystem.h"
#include "Poppy/UI/Fwd.h"

#include <any>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

namespace poppy {

class Toolbar;

class DirViewDelegate {
public:
    virtual ~DirViewDelegate() = default;
};

/// Passed to directory view callbacks
struct DirectoryEntry: std::filesystem::directory_entry {
    explicit DirectoryEntry(std::filesystem::path const& path):
        std::filesystem::directory_entry(path) {}
    DirectoryEntry(std::filesystem::directory_entry const& entry,
                   std::any userData):
        std::filesystem::directory_entry(entry),
        userData(std::move(userData)) {}

    std::any userData;
};

/// Set of callbacks to control the behaviour of a `DirectoryView`
struct DirectoryViewDelegate {
    virtual ~DirectoryViewDelegate() = default;

    /// Action performed when a file is double clicked
    virtual void openFile(DirectoryEntry const&) {}

    /// Callback to handle renaming of directory entries
    virtual void renameEntry(DirectoryEntry const&, std::string /* newName */) {
    }

    /// Callback to handle deletion of directory entries
    virtual void entryDeleted(DirectoryEntry const&) {}

    /// Shall return a CSS icon name for the given file. This will change in the
    /// future when we have proper thumbnails
    virtual std::string selectIcon(DirectoryEntry const&) { return "doc"; }

    /// Used to create user data that will be associated with every directory
    /// entry and passed with it to the other callbacks
    virtual std::any makeUserData(std::filesystem::directory_entry const&) {
        return {};
    }

    /// Use this callback to draw ImGui menu item into the popup menu for the
    /// given file
    virtual void entryContextMenu(DirectoryEntry const&) {}

    /// Display a preview of the directory entry in the preview pane. Return
    /// true if a preview is drawn
    virtual bool displayPreview(DirectoryEntry const&) { return false; }

    ///
    virtual std::string makeDragDropType() { return "FILE_DRAG_DROP"; }

    ///
    virtual std::vector<uint8_t> makeDragDropUserData(
        std::span<DirectoryEntry const>) {
        return {};
    }
};

///
std::span<uint8_t const> peekFileDragDropUserData();

/// General purpose file directory viewer, meant to be used as a subview of a
/// `View`-derived class
class DirectoryView {
public:
    enum class Layout { Columns, Table, List, LAST = List };

    enum class SortCondition { Name, Extension, LAST = Extension };

    /// List of commands that are added to the command palette by `init()`
    enum class CommandType {
        GoUp,
        GoDown,
        GoBack,
        GoForward,
        Rescan,
        LAST = Rescan
    };

    DirectoryView();

    ~DirectoryView();

    void init(std::unique_ptr<DirectoryViewDelegate> delegate,
              bloom::Emitter emitter, CommandPalette& commandPalette);

    Toolbar makeToolbar();

    void display();

    /// Opens \p directory in this viewer
    void openDirectory(std::filesystem::path directory);

    /// \Returns the corresponding command.
    /// \Warning this must be called after `init()` to return nonnull values
    Command const* getCommand(CommandType commandType) const;

    SortCondition sortCondition() const;

    void setSortCondition(SortCondition condition) const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_VIEWS_DIRECTORYVIEW_H
