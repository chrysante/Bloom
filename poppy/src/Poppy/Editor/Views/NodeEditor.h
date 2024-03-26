#ifndef POPPY_EDITOR_VIEWS_NODEEDITOR_H
#define POPPY_EDITOR_VIEWS_NODEEDITOR_H

#include <any>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <range/v3/view.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"

namespace poppy {

/// Generic customizable node editor
class NodeEditor {
public:
    enum class PinType { Input, Output };

    struct PinDesc {
        std::string name;
    };

    struct NodeDesc {
        std::string name;
        vml::float2 position;
        vml::float2 size;
        std::vector<PinDesc> inputs;
        std::vector<PinDesc> outputs;
        std::any userData;
    };

    class Node;

    class Pin {
    public:
        explicit Pin(PinDesc desc, PinType type, Node* target = nullptr):
            _desc(std::move(desc)), _type(type), _target(target) {}

        PinDesc const& desc() const { return _desc; }

        PinType type() const { return _type; }

        void setTarget(Node* target) { _target = target; }

        Node* target() const { return _target; }

        std::string const& name() const { return desc().name; }

    private:
        PinDesc _desc;
        PinType _type;
        Node* _target = nullptr;
    };

    class Node {
    public:
        Node(NodeDesc desc);

        NodeDesc const& desc() const { return _desc; };

        /// \Returns a view over all inputs
        auto inputs() const {
            return _inputs |
                   ranges::views::transform([](auto& p) { return p.get(); });
        }

        /// \Returns a view over all outputs
        auto outputs() const {
            return _outputs |
                   ranges::views::transform([](auto& p) { return p.get(); });
        }

        std::string const& name() const { return desc().name; }

        vml::float2 size() const { return desc().size; }

        void setSize(vml::float2 size) { _desc.size = size; }

        vml::float2 position() const { return desc().position; }

        void setPosition(vml::float2 pos) { _desc.position = pos; }

    private:
        NodeDesc _desc;
        std::vector<std::unique_ptr<Pin>> _inputs;
        std::vector<std::unique_ptr<Pin>> _outputs;
    };

    NodeEditor();

    ~NodeEditor();

    void display();

    void onInput(bloom::InputEvent& event);

    /// Adds \p node to the editor
    void addNode(NodeDesc desc);

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_NODEEDITOR_H
