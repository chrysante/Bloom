#include "Poppy/Views/MaterialNodeController.h"

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
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

bool poppy::isDataTypeCompatible(DataType out, DataType in) {
    if (out == in) {
        return true;
    }
    if ((int)in >= (int)DataType::Float && (int)in <= (int)DataType::Float4 &&
        out == DataType::AnyVector)
    {
        return true;
    }
    if ((int)out >= (int)DataType::Float && (int)out <= (int)DataType::Float4 &&
        in == DataType::AnyVector)
    {
        return true;
    }
    return false;
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
        entries.insert(
            { std::move(name), std::move(deserialize), std::move(menu) });
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
    deserializeMaterialNodeController(std::string_view controllerName,
                                      YAML::Node const& node) {
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
                if ((int)input->userType() >= (int)DataType::Float &&
                    (int)input->userType() <= (int)DataType::Float4)
                {
                    int numComponents =
                        (int)input->userType() - (int)DataType::Float + 1;
                    return ctx.vectorConstant(numComponents, {});
                }
                return nullptr;
            }();
            user->setOperand(inputIndex, operand);
        }
    }
}

OverloadableNodeController::OverloadableNodeController(size_t numInputs,
                                                       DataType defaultType):
    OverloadableNodeController(std::vector<DataType>(numInputs, defaultType)) {}

OverloadableNodeController::OverloadableNodeController(
    std::vector<DataType> inputTypes):
    _inputTypes(std::move(inputTypes)) {}

OverloadableNodeController::OverloadableNodeController(YAML::Node const& root) {
    _inputTypes = root["InputTypes"].as<std::vector<DataType>>();
}

YAML::Node OverloadableNodeController::serialize() const {
    YAML::Node root;
    root["InputTypes"] = _inputTypes;
    return root;
}

bool OverloadableNodeController::isAnyInputUntyped() const {
    return ranges::contains(_inputTypes, DataType::AnyVector);
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
    auto newType = origin ? (DataType)in.origin()->userType() :
                            DataType::AnyVector;
    if (newType == _inputTypes[index]) {
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
