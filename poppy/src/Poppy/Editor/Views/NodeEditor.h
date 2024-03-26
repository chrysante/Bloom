#ifndef POPPY_EDITOR_VIEWS_NODEEDITOR_H
#define POPPY_EDITOR_VIEWS_NODEEDITOR_H

#include <any>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"

namespace poppy {

/// Generic customizable node editor
class NodeEditor {
public:
    struct Pin {
        std::string name;
    };

    struct Node {
        std::string name;
        vml::float2 position;
        vml::float2 size;
        std::vector<Pin> inputs;
        std::vector<Pin> outputs;
        std::any userData;
    };

    NodeEditor();

    ~NodeEditor();

    void display();

    void onInput(bloom::InputEvent& event);

    /// \Returns a view over the nodes in the editor
    std::span<Node const> nodes() const;

    /// Adds \p node to the editor
    void addNode(Node node);

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_NODEEDITOR_H
