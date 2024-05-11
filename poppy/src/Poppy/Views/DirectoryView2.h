#ifndef POPPY_VIEWS_DIRECTORYVIEW2_H
#define POPPY_VIEWS_DIRECTORYVIEW2_H

#include "Bloom/Application/MessageSystem.h"

#include <filesystem>

namespace poppy {

class Toolbar;

class DirectoryView2 {
public:
    enum class Layout { Columns, Table, List, LAST = List };

    DirectoryView2();

    void init(bloom::Emitter emitter);

    Toolbar makeToolbar();

    void display();

    void openDirectory(std::filesystem::path directory);

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_VIEWS_DIRECTORYVIEW2_H
