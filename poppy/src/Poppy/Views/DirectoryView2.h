#ifndef POPPY_VIEWS_DIRECTORYVIEW2_H
#define POPPY_VIEWS_DIRECTORYVIEW2_H

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
struct DirectoryEntry2: std::filesystem::directory_entry {
    explicit DirectoryEntry2(std::filesystem::path const& path):
        std::filesystem::directory_entry(path) {}
    DirectoryEntry2(std::filesystem::directory_entry const& entry,
                    std::any userData):
        std::filesystem::directory_entry(entry),
        userData(std::move(userData)) {}

    std::any userData;
};

/// Set of callbacks to control the behaviour of a `DirectoryView`
struct DirectoryViewDelegate2 {
    virtual ~DirectoryViewDelegate2() = default;

    /// Action performed when a file is double clicked
    virtual void openFile(DirectoryEntry2 const&) {}

    /// Callback to handle renaming of directory entries
    virtual void entryRenamed(DirectoryEntry2 const&,
                              std::string /* newName */) {}

    /// Callback to handle deletion of directory entries
    virtual void entryDeleted(DirectoryEntry2 const&) {}

    /// Shall return a CSS icon name for the given file. This will change in the
    /// future when we have proper thumbnails
    virtual std::string selectIcon(DirectoryEntry2 const&) { return "doc"; }

    /// Used to create user data that will be associated with every directory
    /// entry and passed with it to the other callbacks
    virtual std::any makeUserData(std::filesystem::directory_entry const&) {
        return {};
    }

    /// Use this callback to draw ImGui menu item into the popup menu for the
    /// given file
    virtual void entryContextMenu(DirectoryEntry2 const&) {}

    /// Display a preview of the directory entry in the preview pane. Return
    /// true if a preview is drawn
    virtual bool displayPreview(DirectoryEntry2 const&) { return false; }

    ///
    virtual std::string makeDragDropType() { return "FILE_DRAG_DROP"; }

    ///
    virtual std::vector<uint8_t> makeDragDropUserData(
        std::span<DirectoryEntry2 const>) {
        return {};
    }
};

///
std::span<uint8_t const> peekFileDragDropUserData();

class DirectoryView2 {
public:
    enum class Layout { Columns, Table, List, LAST = List };

    enum class SortCondition { Name, Extension, LAST = Extension };

    DirectoryView2();
    ~DirectoryView2();

    void init(std::unique_ptr<DirectoryViewDelegate2> delegate,
              bloom::Emitter emitter, CommandPalette& commandPalette);

    Toolbar makeToolbar();

    void display();

    void openDirectory(std::filesystem::path directory);

    SortCondition sortCondition() const;

    void setSortCondition(SortCondition condition) const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_VIEWS_DIRECTORYVIEW2_H
