#ifndef POPPY_VIEWS_MATERIALNODECONTROLLER_H
#define POPPY_VIEWS_MATERIALNODECONTROLLER_H

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/queue.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Yaml.h"
#include "Bloom/MaterialCompiler/Fwd.h"
#include "Poppy/Views/NodeEditor.h"

namespace poppy {

using NodeDeserializeFn =
    std::function<std::unique_ptr<nodeEditor::NodeController>(
        YAML::Node const&)>;

using NodeMenuFn = std::function<std::unique_ptr<nodeEditor::Node>()>;

/// Node controller are registered at startup time to a global registry.
/// This structure identifies a registered node controller
struct MaterialNodeRegistryEntry {
    std::string const name;
    NodeDeserializeFn deserialize;
    NodeMenuFn menu;
};

/// \Returns a view over all registered material node controllers
std::span<MaterialNodeRegistryEntry const> getMaterialNodeRegistry();

/// Deserializes a material node controller
std::unique_ptr<nodeEditor::NodeController> deserializeMaterialNodeController(
    YAML::Node const&);

/// Data types in the material editor
enum class DataType { Float, Float2, Float3, Float4, AnyVector, Texture };

/// \Returns `Float` or `FloatN` where `N` is \p numComponents
inline DataType FloatNType(int numComponents) {
    BL_ASSERT(numComponents >= 1 && numComponents <= 4);
    return (DataType)((int)DataType::Float + numComponents - 1);
}

bool isDataTypeCompatible(DataType input, DataType output);

/// Base class of all material node controllers
class MaterialNodeController: public nodeEditor::NodeController {
public:
    struct ValueMap:
        utl::hashmap<nodeEditor::Node*,
                     utl::small_vector<bloom::matc::Value*>> {
        std::span<bloom::matc::Value* const> get(nodeEditor::Node* node) const {
            auto itr = find(node);
            BL_ASSERT(itr != end());
            return itr->second;
        }
        template <typename T = bloom::matc::Value>
        T* get(nodeEditor::Node* node, size_t index) const {
            return bloom::dyncast<T*>(get(node)[index]);
        }
    };

    virtual utl::small_vector<bloom::matc::Value*> createGraphNodes(
        bloom::matc::Context& ctx, bloom::matc::MaterialGraph& graph) const = 0;

    virtual void linkGraphNode(bloom::matc::Context& ctx,
                               ValueMap const& map) const;

private:
    virtual bloom::matc::Value* defaultInputValue(bloom::matc::Context& ctx,
                                                  size_t inputIndex) const {
        return nullptr;
    }
};

/// Base class of material nodes that support multiple input types.
/// This class reacts to `pinConnectionChanged()` and calls
/// `makeOutputDescriptors()` on all nodes that are affected by the change
struct OverloadableNodeController: MaterialNodeController {
public:
    /// \Returns the connected type of the pin at index \p index
    DataType inputType(size_t index) const { return _inputTypes[index]; }

protected:
    explicit OverloadableNodeController(size_t numInputs, DataType defaultType);

    explicit OverloadableNodeController(std::vector<DataType> inputTypes);

    explicit OverloadableNodeController(YAML::Node const& root);

    /// Must be called by any overriding function
    YAML::Node serialize() const override;

    bool isAnyInputUntyped() const;

private:
    void pinConnectionChanged(nodeEditor::Pin& pin) override;

    bool updatePinType(nodeEditor::InputPin& input);

    void notifyUsers();

    std::vector<DataType> _inputTypes;
};

namespace internal {

void registerMaterialNode(std::string name, NodeDeserializeFn deserialize,
                          NodeMenuFn menu);

template <typename NodeType>
inline constexpr auto DeserializeFactory =
    [](YAML::Node const& node) { return std::make_unique<NodeType>(node); };

} // namespace internal

} // namespace poppy

#define REGISTER_MATERIAL_NODE(NodeType, ...)                                  \
    UTL_STATIC_INIT {                                                          \
        poppy::internal::registerMaterialNode(                                 \
            NodeType::Name, poppy::internal::DeserializeFactory<NodeType>,     \
            []() -> std::unique_ptr<poppy::nodeEditor::Node> {                 \
            {                                                                  \
                __VA_ARGS__                                                    \
            }                                                                  \
            return nullptr;                                                    \
        });                                                                    \
    };

#endif // POPPY_VIEWS_MATERIALNODECONTROLLER_H
