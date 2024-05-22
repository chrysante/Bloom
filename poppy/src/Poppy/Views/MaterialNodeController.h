#ifndef POPPY_VIEWS_MATERIALNODECONTROLLER_H
#define POPPY_VIEWS_MATERIALNODECONTROLLER_H

#include <bit>
#include <bitset>
#include <concepts>
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
enum class DataType { Float, Float2, Float3, Float4, Texture };

class DataTypeMask {
public:
    static DataTypeMask const FloatN;
    static DataTypeMask const VectorN;

    constexpr DataTypeMask(nodeEditor::PinUserType userType):
        bits(std::bit_cast<uint64_t>(userType)) {}

    constexpr DataTypeMask(std::same_as<DataType> auto... t):
        DataTypeMask({ { t... } }) {}

    constexpr DataTypeMask(std::span<DataType const> types) {
        for (auto t: types) {
            set(t);
        }
    }

    constexpr void set(DataType t, bool value = true) {
        if (value) {
            bits |= uint64_t(1) << (int)t;
        }
        else {
            bits &= ~(uint64_t(1) << (int)t);
        }
    }

    constexpr bool matches(DataType t) const {
        return contains(t) && count() == 1;
    }

    constexpr bool matches(DataTypeMask m) const { return bits == m.bits; }

    constexpr bool contains(DataType t) const {
        return bits & (uint64_t(1) << (int)t);
    }

    constexpr bool contains(DataTypeMask m) const {
        return (bits & m.bits) == m.bits;
    }

    constexpr bool containsAll(auto... t) const { return (... && contains(t)); }

    constexpr bool containsAny(auto... t) const { return (... || contains(t)); }

    constexpr size_t count() const { return std::popcount(bits); }

    constexpr operator nodeEditor::PinUserType() const {
        return std::bit_cast<nodeEditor::PinUserType>(bits);
    }

    auto range() const {
        return ranges::views::iota(0, 64) | ranges::views::transform([](int n) {
            return DataType(n);
        }) | ranges::views::filter([*this](DataType t) { return contains(t); });
    }

    std::optional<DataType> getSingle() const {
        if (count() == 1) {
            return range().front();
        }
        return std::nullopt;
    }

private:
    uint64_t bits = 0;
};

inline constexpr DataTypeMask DataTypeMask::FloatN = {
    DataType::Float, DataType::Float2, DataType::Float3, DataType::Float4
};
inline constexpr DataTypeMask DataTypeMask::VectorN = { DataType::Float2,
                                                        DataType::Float3,
                                                        DataType::Float4 };

/// \Returns `Float` or `FloatN` where `N` is \p numComponents
inline DataType FloatNType(int numComponents) {
    BL_ASSERT(numComponents >= 1 && numComponents <= 4);
    return (DataType)((int)DataType::Float + numComponents - 1);
}

/// \Returns the number of components in \p type
/// \Pre \p type must be `Float` or `FloatN` where N is either 2, 3 or 4
inline int numComponents(DataType type) {
    using enum DataType;
    BL_ASSERT((int)type >= (int)Float && (int)type <= (int)Float4);
    return (int)type - (int)Float + 1;
}

bool isDataTypeCompatible(DataTypeMask input, DataTypeMask output);

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
    virtual bloom::matc::Value* defaultInputValue(
        bloom::matc::Context&, [[maybe_unused]] size_t inputIndex) const {
        return nullptr;
    }
};

/// Base class of material nodes that support multiple input types.
/// This class reacts to `pinConnectionChanged()` and calls
/// `makeOutputDescriptors()` on all nodes that are affected by the change
struct OverloadableNodeController: MaterialNodeController {
public:
    /// \Returns the connected type of the pin at index \p index
    DataTypeMask inputType(size_t index) const { return _inputTypes[index]; }

    /// \Returns the default type of the pin at index \p index
    DataTypeMask defaultType(size_t index) const {
        return _defaultTypes[index];
    }

    /// \Returns the connected input types
    std::span<DataTypeMask const> inputTypes() const { return _inputTypes; }

    /// \Returns the number of inputs
    size_t numInputs() const { return _inputTypes.size(); }

    /// \Returns `true` if any input has more than a single type specified
    bool isAnyInputUntyped() const;

protected:
    explicit OverloadableNodeController(size_t numInputs,
                                        DataTypeMask defaultType);

    explicit OverloadableNodeController(std::vector<DataTypeMask> defaultTypes);

    explicit OverloadableNodeController(YAML::Node const& root);

    /// Must be called by any overriding function
    YAML::Node serialize() const override;

private:
    void pinConnectionChanged(nodeEditor::Pin& pin) override;

    bool updatePinType(nodeEditor::InputPin& input);

    void notifyUsers();

    std::vector<DataTypeMask> _defaultTypes;
    std::vector<DataTypeMask> _inputTypes;
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

#ifndef BLOOM_NO_SERIALIZATION

template <>
struct YAML::convert<poppy::DataTypeMask> {
    static Node encode(poppy::DataTypeMask mask) {
        Node node;
        ranges::for_each(mask.range(),
                         [&](poppy::DataType t) { node.push_back(t); });
        return node;
    }
    static bool decode(Node const& node, poppy::DataTypeMask& mask) {
        for (auto& field: node) {
            mask.set(field.as<poppy::DataType>());
        }
        return true;
    }
};

#endif // BLOOM_NO_SERIALIZATION

#endif // POPPY_VIEWS_MATERIALNODECONTROLLER_H
