#ifndef BLOOM_APPLICATION_RESOURCEUTIL_H
#define BLOOM_APPLICATION_RESOURCEUTIL_H

#include <filesystem>
#include <functional>
#include <optional>
#include <span>

#include "Bloom/Core/Base.h"

namespace bloom {

/// \Returns the path of the resource directory
BLOOM_API std::filesystem::path resourceDir();

/// \Returns the path of the library directory
BLOOM_API std::filesystem::path libraryDir();

/// Displays a system "save" panel
BLOOM_API void showSavePanel(
    std::function<void(std::filesystem::path const&)> completion);

/// Options for `showOpenPanel()`
struct OpenPanelDescription {
    ///
    bool resolvesAliases = true;

    ///
    bool canChooseDirectories = false;

    /// Set to `true` to only allow selecting multiple files
    bool allowsMultipleSelection = false;

    /// Set to `false` to only allow choosing directories
    bool canChooseFiles = true;
};

/// Displays a system "open" panel
BLOOM_API void showOpenPanel(
    OpenPanelDescription const& desc,
    std::function<void(std::span<std::filesystem::path const>)> completion);

/// \overload
BLOOM_API void showOpenPanel(
    OpenPanelDescription const& desc,
    std::function<void(std::filesystem::path const&)> completion);

} // namespace bloom

#endif // BLOOM_APPLICATION_RESOURCEUTIL_H
