#ifndef POPPY_VIEWS_DIRECTORYVIEW_H
#define POPPY_VIEWS_DIRECTORYVIEW_H

#include "Bloom/Application/MessageSystem.h"
#include "Poppy/UI/Fwd.h"

#include <any>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include <utl/vector.hpp>

namespace poppy {

class Toolbar;

///
class DirectoryEntry {
public:
    virtual ~DirectoryEntry() = default;

    virtual bool isDirectory() const = 0;

    virtual void erase() = 0;

    virtual void rename(std::string newName) = 0;

    virtual void relocate(std::filesystem::path newLocation) = 0;

    virtual std::string_view displayName() const = 0;

    virtual std::string extension() const { return path().extension(); }

    virtual std::filesystem::path path() const = 0;

    virtual std::vector<std::unique_ptr<DirectoryEntry>> makeChildEntries() = 0;

    virtual void createFolder(std::string name) = 0;

    virtual void open() const {}

    virtual std::string iconName() const { return "doc"; }

    virtual void contextMenu() {}

    virtual bool displayPreview() { return false; }

    virtual utl::small_vector<char> imguiDragDropUserData() const { return {}; }
};

struct DeleteConfirmResponse {
    static DeleteConfirmResponse const MayDelete;

    static DeleteConfirmResponse ConfirmNumItems(size_t numItems);

    static DeleteConfirmResponse Confirm(std::string message);

    DeleteConfirmResponse() = default;

    std::string const& message() const { return _message; }

    bool mayDelete() const { return _mayDelete; }

private:
    explicit DeleteConfirmResponse(bool mayDelete, std::string message):
        _mayDelete(mayDelete), _message(std::move(message)) {}

    bool _mayDelete = true;
    std::string _message;
};

///
class DirectoryViewDelegate {
public:
    virtual ~DirectoryViewDelegate() = default;

    virtual std::unique_ptr<DirectoryEntry> makeEntry(
        std::filesystem::path path) = 0;

    virtual char const* imguiDragDropType() const { return "FILE_DRAG_DROP"; }

    virtual DeleteConfirmResponse confirmDeletion(
        [[maybe_unused]] std::span<DirectoryEntry const* const> entries) const {
        return DeleteConfirmResponse::MayDelete;
    }
};

struct DirectoryViewDragDropItem {
    std::filesystem::path path;
    utl::small_vector<char> userData;
};

///
std::vector<DirectoryViewDragDropItem> peekFileDragDropData();

/// General purpose file directory viewer, meant to be used as a subview of a
/// `View`-derived class
class DirectoryView {
public:
    enum class Layout { Columns, Table, List };

    enum class SortCondition { Name, Extension };

    /// List of commands that are added to the command palette by `init()`
    enum class CommandType {
        GoUp,
        GoDown,
        GoBack,
        GoForward,
        Rescan,
        CreateFolder
    };

    DirectoryView();

    ~DirectoryView();

    void init(std::unique_ptr<DirectoryViewDelegate> delegate,
              bloom::Emitter emitter, CommandPalette& commandPalette);

    Toolbar makeToolbar();

    void display();

    /// Opens \p directory in this view
    void openDirectory(std::filesystem::path path);

    /// \Returns the deepest currently open directory
    std::filesystem::path currentDirectory() const;

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
