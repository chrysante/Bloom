#ifndef POPPY_EDITOR_VIEWS_NODEEDITOR_H
#define POPPY_EDITOR_VIEWS_NODEEDITOR_H

#include <any>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Core/Dyncast.h"

namespace poppy::nodeEditor {

class Pin;
class InputPin;
class OutputPin;

enum class PinType { Pin, InputPin, OutputPin, LAST = OutputPin };

} // namespace poppy::nodeEditor

UTL_DYNCAST_DEFINE(poppy::nodeEditor::Pin, poppy::nodeEditor::PinType::Pin,
                   void, Abstract)
UTL_DYNCAST_DEFINE(poppy::nodeEditor::InputPin,
                   poppy::nodeEditor::PinType::InputPin, poppy::nodeEditor::Pin,
                   Concrete)
UTL_DYNCAST_DEFINE(poppy::nodeEditor::OutputPin,
                   poppy::nodeEditor::PinType::OutputPin,
                   poppy::nodeEditor::Pin, Concrete)

namespace poppy::nodeEditor {

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

/// Base class of pins
class Pin {
public:
    PinDesc const& desc() const { return _desc; }

    PinType type() const { return _type; }

    std::string const& name() const { return desc().name; }

    Node& parent() { return *_parent; }

    Node const& parent() const { return *_parent; }

protected:
    explicit Pin(PinType type, PinDesc const& desc, Node& parent):
        _desc(desc), _type(type), _parent(&parent) {}

private:
    friend inline PinType dyncast_get_type(Pin const& pin) {
        return pin.type();
    }

    PinDesc _desc;
    PinType _type;
    Node* _parent;
};

class InputPin: public Pin {
public:
    explicit InputPin(PinDesc const& desc, Node& parent):
        Pin(PinType::InputPin, desc, parent) {}

    OutputPin* origin() const { return _origin; }

    void setOrigin(OutputPin* origin);

private:
    OutputPin* _origin = nullptr;
};

class OutputPin: public Pin {
public:
    explicit OutputPin(PinDesc const& desc, Node& parent):
        Pin(PinType::OutputPin, desc, parent) {}

    std::span<Pin* const> targets() const { return _targets; }

    void addTarget(Pin* target);

    bool isTarget(Pin const* target) const;

    void removeTarget(Pin* target);

    void clearTargets();

private:
    utl::small_vector<Pin*> _targets;
};

template <typename T>
using UniquePtr = std::unique_ptr<T, utl::dyn_deleter>;

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
    std::vector<nodeEditor::UniquePtr<nodeEditor::InputPin>> _inputs;
    std::vector<nodeEditor::UniquePtr<nodeEditor::OutputPin>> _outputs;
};

} // namespace poppy::nodeEditor

namespace poppy {

/// Generic customizable node editor
class NodeEditor {
public:
    NodeEditor();

    ~NodeEditor();

    void display();

    void onInput(bloom::InputEvent& event);

    /// Adds \p node to the editor
    void addNode(nodeEditor::NodeDesc desc);

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_NODEEDITOR_H
