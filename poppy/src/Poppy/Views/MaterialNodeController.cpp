#include "Poppy/Views/MaterialNodeController.h"

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Yaml.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace poppy;
using namespace bloom;
using namespace matc;
using namespace nodeEditor;
using namespace vml::short_types;
using namespace ranges::views;

bool poppy::isDataTypeCompatible(DataTypeMask out, DataTypeMask in) {
    return in.contains(out) || out.contains(in);
}

namespace {

template <auto Proj, template <class T> class Hash = std::hash>
struct ProjectionHash {
    using is_transparent = void;
    using ProjT = decltype(Proj);

    template <typename T>
        requires std::invocable<ProjT, T const&>
    size_t operator()(T const& t) const {
        using M = std::decay_t<std::invoke_result_t<ProjT, T const&>>;
        return Hash<M>{}(std::invoke(Proj, t));
    }
    template <typename T>
    size_t operator()(T const& t) const {
        return Hash<std::decay_t<T const&>>{}(t);
    }
};

template <auto Proj>
struct ProjectionEqual {
    using is_transparent = void;
    using ProjT = decltype(Proj);

    template <typename T>
        requires std::invocable<ProjT, T const&>
    bool operator()(T const& a, T const& b) const {
        return std::equal_to<>{}(std::invoke(Proj, a), std::invoke(Proj, b));
    }

    template <typename T>
        requires std::invocable<ProjT, T const&>
    bool operator()(T const& t, auto const& other) const {
        return std::equal_to<>{}(std::invoke(Proj, t), other);
    }

    template <typename T>
        requires std::invocable<ProjT, T const&>
    bool operator()(auto const& other, T const& t) const {
        return operator()(t, other);
    }
};

struct NodeRegistry {
    using Entry = MaterialNodeRegistryEntry;

    void add(std::string name, NodeDeserializeFn deserialize, NodeMenuFn menu) {
        bool success = entries
                           .insert({ std::move(name), std::move(deserialize),
                                     std::move(menu) })
                           .second;
        BL_ASSERT(success, "Failed to register material node");
    }

    utl::hashset<Entry, ProjectionHash<&Entry::name, utl::hash>,
                 ProjectionEqual<&Entry::name>>
        entries;
};

static NodeRegistry gRegistry;

} // namespace

std::span<MaterialNodeRegistryEntry const> poppy::getMaterialNodeRegistry() {
    return gRegistry.entries.values();
}

std::unique_ptr<nodeEditor::NodeController> poppy::
    deserializeMaterialNodeController(YAML::Node const& node) {
    auto controllerName = node["Type"].as<std::string>();
    auto itr = gRegistry.entries.find(controllerName);
    if (itr == gRegistry.entries.end()) {
        return nullptr;
    }
    return itr->deserialize(node);
}

void poppy::internal::registerMaterialNode(std::string name,
                                           NodeDeserializeFn deserialize,
                                           NodeMenuFn menu) {
    gRegistry.add(std::move(name), std::move(deserialize), std::move(menu));
}

void MaterialNodeController::linkGraphNode(Context& ctx,
                                           ValueMap const& map) const {
    auto values = map.get(node());
    for (auto [valueIndex, value]: values | enumerate) {
        auto* user = dyncast<User*>(value);
        if (!user) {
            continue;
        }
        for (auto [inputIndex, input]: node()->inputs() | enumerate) {
            if (inputIndex >= user->numOperands() || user->operand(inputIndex))
            {
                continue;
            }
            auto* operand = [&]() -> Value* {
                if (input->origin()) {
                    auto* o = input->origin();
                    return map.get(&o->parent(), o->indexInParent());
                }
                if (input->hasFlags(PinFlags::Optional)) {
                    auto* value = defaultInputValue(ctx, inputIndex);
                    BL_ASSERT(value, "Missing default value");
                    return value;
                }
                throw std::runtime_error(
                    utl::strcat("Missing link on node: ", node()->name()));
            }();
            user->setOperand(inputIndex, operand);
        }
    }
}

OverloadableNodeController::OverloadableNodeController(
    size_t numInputs, DataTypeMask defaultType):
    OverloadableNodeController(
        std::vector<DataTypeMask>(numInputs, defaultType)) {}

OverloadableNodeController::OverloadableNodeController(
    std::vector<DataTypeMask> defaultTypes):
    _defaultTypes(defaultTypes), _inputTypes(std::move(defaultTypes)) {}

OverloadableNodeController::OverloadableNodeController(YAML::Node const& root) {
    _defaultTypes = root["DefaultTypes"].as<std::vector<DataTypeMask>>();
    _inputTypes = root["InputTypes"].as<std::vector<DataTypeMask>>();
}

YAML::Node OverloadableNodeController::serialize() const {
    YAML::Node root;
    root["DefaultTypes"] = _defaultTypes;
    root["InputTypes"] = _inputTypes;
    return root;
}

bool OverloadableNodeController::isAnyInputUntyped() const {
    return ranges::any_of(_inputTypes,
                          [](DataTypeMask t) { return t.count() > 1; });
}

void OverloadableNodeController::pinConnectionChanged(nodeEditor::Pin& pin) {
    auto* in = bloom::dyncast<InputPin*>(&pin);
    if (!in) {
        return;
    }
    updatePinType(*in);
    notifyUsers();
}

bool OverloadableNodeController::updatePinType(InputPin& in) {
    size_t index = in.indexInParent();
    BL_ASSERT(index < _inputTypes.size());
    auto* origin = in.origin();
    auto newType = origin ? DataTypeMask(in.origin()->userType()) :
                            defaultType(index);
    if (newType == inputType(index)) {
        return false;
    }
    _inputTypes[index] = newType;
    auto descs = makeOutputDescriptors();
    for (auto [pin, desc]: ranges::views::zip(node()->outputs(), descs)) {
        pin->setDesc(desc);
    }
    return true;
}

void OverloadableNodeController::notifyUsers() {
    utl::queue<OverloadableNodeController*> queue = { this };
    utl::hashset<OverloadableNodeController*> visited = { this };
    while (!queue.empty()) {
        auto* controller = queue.pop();
        for (auto* outpin: controller->node()->outputs()) {
            for (auto* target: outpin->targets()) {
                auto* targetController =
                    dynamic_cast<OverloadableNodeController*>(
                        target->parent().controller());
                if (targetController &&
                    targetController->updatePinType(*target) &&
                    visited.insert(targetController).second)
                {
                    queue.push(targetController);
                }
            }
        }
    }
}
