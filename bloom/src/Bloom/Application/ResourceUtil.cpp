#include "Bloom/Application/ResourceUtil.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace bloom;

void bloom::showOpenPanel(
    OpenPanelDescription const& desc,
    std::function<void(std::filesystem::path const&)> completion) {
    showOpenPanel(desc, [=](std::span<std::filesystem::path const> paths) {
        completion(paths.empty() ? std::filesystem::path{} : paths.front());
    });
}
