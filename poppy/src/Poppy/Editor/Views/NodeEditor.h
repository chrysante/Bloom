#ifndef POPPY_EDITOR_VIEWS_NODEEDITOR_H
#define POPPY_EDITOR_VIEWS_NODEEDITOR_H

#include <any>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
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
    /// The description from which this pin was created
    PinDesc const& desc() const { return _desc; }

    /// Runtime type of this pin
    PinType type() const { return _type; }

    /// Label name
    std::string const& name() const { return desc().name; }

    /// The node in which this pin exists
    Node& parent() { return *_parent; }

    /// \overload
    Node const& parent() const { return *_parent; }

    /// Computes the index of this pin in its parent node
    size_t indexInParent() const;

    /// Removes all links from this pin
    void clearLinks();

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

///
class InputPin: public Pin {
public:
    explicit InputPin(PinDesc const& desc, Node& parent):
        Pin(PinType::InputPin, desc, parent) {}

    /// \Returns the output that is linked to this pin
    OutputPin* origin() const { return _origin; }

    /// Sets the output that is linked to this pin
    void setOrigin(OutputPin* origin);

private:
    OutputPin* _origin = nullptr;
};

///
class OutputPin: public Pin {
public:
    explicit OutputPin(PinDesc const& desc, Node& parent):
        Pin(PinType::OutputPin, desc, parent) {}

    /// \Returns a view over the input pins that are linked to this putput
    std::span<InputPin* const> targets() const { return _targets; }

    /// Adds \p pin as a target of this pin. This does not update \p target
    void addTarget(InputPin* pin);

    /// \Returns `true` if a link from this pin to \p target exists
    bool isTarget(InputPin const* target) const;

    /// Removes the link to \p target
    /// This also updates \p target
    void removeTarget(InputPin* target);

    /// Removes all targets. This also updates all targets
    void clearTargets();

private:
    utl::small_vector<InputPin*> _targets;
};

template <typename T>
using UniquePtr = std::unique_ptr<T, utl::dyn_deleter>;

///
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

    /// \Returns the set of nodes that is directly linked to any output of this
    /// node
    utl::hashset<Node*> successors();

    /// \overload
    utl::hashset<Node const*> successors() const;

    /// \Returns the set of nodes that is directly linked to any input of this
    /// node
    utl::hashset<Node*> predecessors();

    /// \overload
    utl::hashset<Node const*> predecessors() const;

    std::string const& name() const { return desc().name; }

    vml::float2 size() const { return desc().size; }

    void setSize(vml::float2 size) { _desc.size = size; }

    vml::float2 position() const { return desc().position; }

    void setPosition(vml::float2 pos) { _desc.position = pos; }

private:
    friend class Pin;

    NodeDesc _desc;
    std::vector<nodeEditor::UniquePtr<nodeEditor::InputPin>> _inputs;
    std::vector<nodeEditor::UniquePtr<nodeEditor::OutputPin>> _outputs;
};

/// Underlying model of the node editor
class Graph {
public:
    /// \Returns a view over the nodes in this graph
    auto nodes() {
        return _nodes | ranges::views::transform(
                            [](auto& p) -> Node* { return p.get(); });
    }

    /// \overload
    auto nodes() const {
        return _nodes | ranges::views::transform(
                            [](auto& p) -> Node const* { return p.get(); });
    }

    /// \Returns `true` if there are any cycles in this graph
    bool hasCycles() const;

    /// \Returns all nodes that have no inputs
    utl::small_vector<Node*> sources();

    /// \overload
    utl::small_vector<Node const*> sources() const;

    /// \Returns all nodes that have no outputs
    utl::small_vector<Node*> sinks();

    /// \overload
    utl::small_vector<Node const*> sinks() const;

    ///
    void add(std::unique_ptr<Node> node) { _nodes.push_back(std::move(node)); }

private:
    std::vector<std::unique_ptr<Node>> _nodes;
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

    /// \Returns the underlying graph
    nodeEditor::Graph const& graph() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_NODEEDITOR_H
