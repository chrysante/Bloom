#ifndef POPPY_VIEWS_NODEEDITOR_H
#define POPPY_VIEWS_NODEEDITOR_H

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/common.hpp>
#include <utl/function_view.hpp>
#include <utl/hashtable.hpp>
#include <utl/uuid.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Core/Dyncast.h"
#include "Poppy/UI/Fwd.h"

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

enum class PinFlags { Optional = 1 << 0 };

UTL_ENUM_OPERATORS(PinFlags)

struct PinDesc {
    std::string name;
    PinFlags flags;
};

/// Node controller interface. Objects of derived classes can be attached to any
/// node to customize its behaviour
class NodeController {
public:
    virtual ~NodeController() = default;

    /// Called when serializing the containing graph. Deserialization is done
    /// through a callback to `NodeEditorGraph::deserialize`, because it must
    /// determine the static type of the node controller
    virtual YAML::Node serialize() const { return YAML::Node{}; }

    /// Callback to display content in the node body
    virtual void bodyContent() {}

    /// Callback to display the content of the nodes context menu
    virtual void contextMenuContent() {}
};

/// Descriptor structure used to create nodes
struct NodeDesc {
    /// Unique identifier of this node. This will be set by the model if not set
    /// by the user
    utl::uuid ID;

    /// The label of the node in the editor
    std::string name;

    /// The position at which the node will be created
    vml::float2 position;

    /// Minimum size below which the node cannot be resized
    vml::float2 minSize;

    /// The size with which the node will be created.
    vml::float2 size;

    ///
    std::vector<PinDesc> inputs;

    ///
    std::vector<PinDesc> outputs;

    /// The color of the node body
    vml::float4 color = { 1, 0, 0.5, 1 };

    ///
    std::unique_ptr<NodeController> controller;
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
void link(OutputPin& output, InputPin& input);

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
    friend void link(OutputPin&, InputPin&);

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
    friend class InputPin;
    friend void link(OutputPin&, InputPin&);

    bool removeTargetWeak(InputPin* pin);

    utl::small_vector<InputPin*> _targets;
};

///
class Node {
public:
    explicit Node(NodeDesc desc);

    /// Nodes need address stability and are therefore not copyable
    Node(Node const&) = delete;

    /// Calls `clearLinks` on all input and output pins
    ~Node();

    NodeDesc const& desc() const { return _desc; };

    /// \Returns the attached controller. This may be null
    NodeController* controller() const { return desc().controller.get(); }

    /// \Returns a view over all inputs
    auto inputs() const {
        return _inputs |
               ranges::views::transform([](auto& p) { return p.get(); });
    }

    /// \Returns the input pin at index \p index
    InputPin* input(size_t index) const {
        BL_EXPECT(index < _inputs.size());
        return _inputs[index].get();
    }

    /// \Returns a view over all outputs
    auto outputs() const {
        return _outputs |
               ranges::views::transform([](auto& p) { return p.get(); });
    }

    /// \Returns the output pin at index \p index
    OutputPin* output(size_t index) const {
        BL_EXPECT(index < _outputs.size());
        return _outputs[index].get();
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

    /// \Returns the unique ID of this node
    utl::uuid ID() const { return desc().ID; }

    /// \Returns the name of this node
    std::string const& name() const { return desc().name; }

    /// \Returns the size of this node
    vml::float2 size() const { return desc().size; }

    /// Sets the size of this node to \p size
    void setSize(vml::float2 size) { _desc.size = size; }

    /// \Returns the minimum size of this node
    vml::float2 minSize() const { return desc().minSize; }

    /// \Returns the position of this node
    vml::float2 position() const { return desc().position; }

    /// Sets the position of this node to \p position
    void setPosition(vml::float2 position) { _desc.position = position; }

private:
    friend class Pin;

    NodeDesc _desc;
    std::vector<bloom::DynUniquePtr<nodeEditor::InputPin>> _inputs;
    std::vector<bloom::DynUniquePtr<nodeEditor::OutputPin>> _outputs;
};

} // namespace poppy::nodeEditor

namespace poppy {

/// Underlying document model of the node editor
class NodeEditorGraph {
public:
    using Node = nodeEditor::Node;

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

    ///
    bool empty() const { return _nodes.empty(); }

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

    /// Inserts the node \p node into the graph
    void add(std::unique_ptr<Node> node) { _nodes.push_back(std::move(node)); }

    /// \Returns a YAML node representing this document
    YAML::Node serialize() const;

    /// Deserializes the document stored in \p node
    bool deserialize(
        YAML::Node const& node,
        utl::function_view<
            std::unique_ptr<nodeEditor::NodeController>(YAML::Node const&)>
            deserializeController = {});

    /// Deletes all nodes for which \p predicate returns true
    void deleteIf(utl::function_view<bool(Node const&)> predicate);

    /// Moves the node \p node to the front. This changes the display order of
    /// the nodes
    void orderFront(Node* node);

private:
    std::vector<std::unique_ptr<Node>> _nodes;
};

/// Callbacks to customize the behaviour of a node editor
struct NodeEditorDelegate {
    /// Content callback for the editor background popup menu
    std::function<void(vml::float2 position)> backgroundPopupMenu;
};

/// Generic customizable node editor
class NodeEditor {
public:
    /// Registers commands with \p commandPalette
    explicit NodeEditor(CommandPalette& commandPalette);

    ~NodeEditor();

    void setDelegate(NodeEditorDelegate delegate);

    void setGraph(NodeEditorGraph graph);

    void display();

    void onInput(bloom::InputEvent& event);

    /// Adds \p node to the editor
    void addNode(nodeEditor::NodeDesc desc);

    /// \Returns the underlying graph
    NodeEditorGraph const& model() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace poppy

#endif // POPPY_VIEWS_NODEEDITOR_H
