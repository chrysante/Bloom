#pragma once

#include <filesystem>
#include <optional>

#include <utl/functional.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Base.hpp"

namespace bloom {

BLOOM_API std::filesystem::path resourceDir();

BLOOM_API std::filesystem::path libraryDir();

BLOOM_API void showSavePanel(
    utl::function<void(std::filesystem::path const&)> completion);

struct OpenPanelDescription {
    bool resolvesAliases         = true;
    bool canChooseDirectories    = false;
    bool allowsMultipleSelection = false;
    bool canChooseFiles          = true;
};

BLOOM_API void showOpenPanel(
    OpenPanelDescription const& desc,
    utl::function<void(utl::vector<std::filesystem::path> const&)> completion);

BLOOM_API inline void showOpenPanel(
    OpenPanelDescription const& desc,
    utl::function<void(std::filesystem::path const&)> completion) {
    utl::function<void(utl::vector<std::filesystem::path> const&)> callback =
        [=](utl::vector<std::filesystem::path> const& paths) {
        completion(paths.empty() ? std::filesystem::path{} : paths.front());
    };
    showOpenPanel(desc, callback);
}

BLOOM_API std::optional<std::string> readFileText(std::filesystem::path const&);

BLOOM_API std::optional<utl::vector<char>> readFileBinary(
    std::filesystem::path const&);

} // namespace bloom
