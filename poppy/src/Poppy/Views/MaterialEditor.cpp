#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <magic_enum.hpp>
#include <utl/queue.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;
using namespace nodeEditor;
using namespace matc;
using namespace vml::short_types;
using namespace ranges::views;

static Editor* gEditor = nullptr;

namespace {

static constexpr std::array<float4, 3> PinChannelColors = {
    { { 1, 0, 0, 1 }, { .4, .9, .3, 1 }, { 0, 0.5, 1, 1 } }
};

enum class DataType { Float, Float2, Float3, Float4, AnyVector, Texture };

enum class NodeType {
    Output,
    Input,
    Constant,
    Append,
    Split,
    BuiltinFunction,
    Arithmetic,
    TextureSampler
};

enum class InputKind {
    TextureCoords,
    Normals,
    Tangents,
    Binormals,
    WorldPosition
};

enum class FunctionType { Normalize, Sqrt };

enum class ArithmeticOp { Add, Sub, Mul, Div, LAST = Div };

static Type const* translateType(Context& ctx, DataType dataType) {
    using enum DataType;
    switch (dataType) {
    case DataType::Float:
        [[fallthrough]];
    case DataType::Float2:
        [[fallthrough]];
    case DataType::Float3:
        [[fallthrough]];
    case DataType::Float4: {
        int numComponents = (int)dataType - (int)DataType::Float + 1;
        return ctx.vectorType(numComponents);
    }
    case AnyVector:
        throw std::runtime_error("Compiling value of AnyVector type");
    case Texture:
        BL_UNIMPLEMENTED();
    }
}

struct MaterialNodeController: NodeController {
    struct ValueMap: utl::hashmap<Node*, utl::small_vector<Value*>> {
        std::span<Value* const> get(Node* node) const {
            auto itr = find(node);
            BL_ASSERT(itr != end());
            return itr->second;
        }
        template <typename T = Value>
        T* get(Node* node, size_t index) const {
            return dyncast<T*>(get(node)[index]);
        }
    };

    virtual utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const = 0;

    virtual void linkGraphNode(Context& ctx, ValueMap const& map) const {
        auto values = map.get(node());
        for (auto [valueIndex, value]: values | enumerate) {
            auto* user = dyncast<User*>(value);
            if (!user) {
                continue;
            }
            for (auto [inputIndex, input]: node()->inputs() | enumerate) {
                if (inputIndex >= user->numOperands() ||
                    user->operand(inputIndex))
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
};

struct OverloadableNodeController: MaterialNodeController {
protected:
    explicit OverloadableNodeController(size_t numInputs, DataType defaultType):
        OverloadableNodeController(
            std::vector<DataType>(numInputs, defaultType)) {}

    explicit OverloadableNodeController(std::vector<DataType> inputTypes):
        inputTypes(std::move(inputTypes)) {}

    explicit OverloadableNodeController(YAML::Node const& root) {
        inputTypes = root["InputTypes"].as<std::vector<DataType>>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["InputTypes"] = inputTypes;
        return root;
    }

    bool isAnyInputUntyped() const {
        return ranges::contains(inputTypes, DataType::AnyVector);
    }

    std::vector<DataType> inputTypes;

private:
    void pinConnectionChanged(Pin& pin) override {
        auto* in = dyncast<InputPin*>(&pin);
        if (!in) {
            return;
        }
        updatePinType(*in);
        notifyUsers();
    }

    bool updatePinType(InputPin& in) {
        size_t index = in.indexInParent();
        BL_ASSERT(index < inputTypes.size());
        auto* origin = in.origin();
        auto newType = origin ? (DataType)in.origin()->userType() :
                                DataType::AnyVector;
        if (newType == inputTypes[index]) {
            return false;
        }
        inputTypes[index] = newType;
        auto descs = makeOutputDescriptors();
        for (auto [pin, desc]: zip(node()->outputs(), descs)) {
            pin->setDesc(desc);
        }
        return true;
    }

    void notifyUsers() {
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
};

struct OutputNodeController: MaterialNodeController {
    static NodeDesc Make(float2 position) {
        return { .name = "Output",
                 .position = position,
                 .controller = std::make_unique<OutputNodeController>() };
    }

    OutputNodeController() = default;

    explicit OutputNodeController(YAML::Node const&) {}

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = NodeType::Output;
        return node;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return {
            { .name = "Base Color",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float3 },
            { .name = "Emissive",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float3 },
            { .name = "Roughness",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float },
            { .name = "Metallic",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float },
            { .name = "Ambient Occlusion",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float },
            { .name = "Normal",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float3 },
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        auto* type = ctx.aggregateType(std::vector<Type const*>{
            ctx.float3Type(),
            ctx.float3Type(),
            ctx.floatType(),
            ctx.floatType(),
            ctx.floatType(),
            ctx.float3Type(),
        });
        auto* value = graph.addValue<AggregateBuilder>(type);
        graph.setRoot(value);
        return { value };
    }

    vml::float4 nodeColor() override { return { 0.1, 0.1, 0.1, 1 }; }
};

struct ConstantController: MaterialNodeController {
    static NodeDesc Make(float2 position, DataType dataType) {
        using enum DataType;
        NodeDesc desc = {
            .name = utl::strcat(magic_enum::enum_name(dataType), " Constant"),
            .position = position,
            .controller = std::make_unique<ConstantController>(dataType)
        };
        switch (dataType) {
        case Float3:
            [[fallthrough]];
        case Float4:
            [[fallthrough]];
        case Texture:
            desc.resizable = true;
            break;
        default:
            break;
        }
        return desc;
    }

    explicit ConstantController(DataType dataType): dataType(dataType) {}

    explicit ConstantController(YAML::Node const& node) {
        dataType = node["DataType"].as<DataType>();
        value = node["Value"].as<float4>();
        textureAsset = node["Texture"].as<AssetHandle>();
        textype = node["TextureType"].as<TextureSubType>();
    }

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = NodeType::Constant;
        node["DataType"] = dataType;
        node["Value"] = value;
        node["Texture"] = textureAsset;
        node["TextureType"] = textype;
        return node;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        using enum DataType;
        switch (dataType) {
        case Float:
            return {
                { .name = "Value", .userType = (PinUserType)Float },
            };
        case Float2:
            return {
                { .name = "Value", .userType = (PinUserType)Float2 },
                { .name = "Red",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[0] },
                { .name = "Green",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[1] },
            };
        case Float3:
            return {
                { .name = "Value", .userType = (PinUserType)Float3 },
                { .name = "Red",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[0] },
                { .name = "Green",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[1] },
                { .name = "Blue",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[2] },
            };
        case Float4:
            return {
                { .name = "Value", .userType = (PinUserType)Float3 },
                { .name = "Red",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[0] },
                { .name = "Green",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[1] },
                { .name = "Blue",
                  .userType = (PinUserType)Float,
                  .color = PinChannelColors[2] },
                { .name = "Alpha",
                  .userType = (PinUserType)Float,
                  .flags = PinFlags::Optional },
            };
        case AnyVector:
            BL_UNREACHABLE();
        case Texture:
            return {
                { .name = "Texture", .userType = (PinUserType)Texture },
            };
        }
    }

    utl::small_vector<Value*> createGraphNodes(Context& ctx,
                                               MaterialGraph&) const override {
        using enum DataType;
        switch (dataType) {
        case DataType::Float:
            [[fallthrough]];
        case DataType::Float2:
            [[fallthrough]];
        case DataType::Float3:
            [[fallthrough]];
        case DataType::Float4: {
            int numComponents = (int)dataType - (int)DataType::Float + 1;
            return { ctx.vectorConstant(numComponents, value) };
        }
        case AnyVector:
            BL_UNREACHABLE();
        case Texture:
            return { ctx.textureConstant(textype, textureAsset) };
        }
    }

    bool alwaysDisplayInputNames() const override { return false; }

    bool alwaysDisplayOutputNames() const override { return false; }

    float4 nodeColor() override { return { 0.2, .8, .2, 1 }; }

    float2 preferredContentSize() const override {
        auto& style = ImGui::GetStyle();
        float itemHeight = ImGui::GetFontSize() + 2 * style.FramePadding.y +
                           style.ItemSpacing.y;
        using enum DataType;
        switch (dataType) {
        case Float:
            return { 70, itemHeight };
        case Float2:
            return { 70, itemHeight };
        case Float3:
            return { 150, 180 };
        case Float4:
            return { 150, 180 };
        case AnyVector:
            BL_UNREACHABLE();
        case Texture:
            return float2{ 100, 100 } + float2(0, itemHeight);
        }
    }

    void bodyContent() override {
        auto colFlags =
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder |
            ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoOptions;
        switch (dataType) {
        case DataType::Float:
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("###Value", &value.x, 0.001);
            break;
        case DataType::Float2:
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("###ValueR", &value.x, 0.001);
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("###ValueG", &value.y, 0.001);
            break;
        case DataType::Float3:
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::ColorPicker3("###Value", value.data(), colFlags);
            break;
        case DataType::Float4:
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::ColorPicker4("###Value", value.data(), colFlags);
            break;
        case DataType::AnyVector:
            BL_UNREACHABLE();
        case DataType::Texture: {
            if (auto asset = acceptAssetDragDrop(AssetType::Texture2D)) {
                textureAsset = *asset;
                texHandle = {};
                triedToGetTexture = false;
            }
            if (!texHandle && !triedToGetTexture) {
                Reference<Asset> asset;
                gEditor->dispatch(DispatchToken::Now,
                                  AMGetAsset{ .handle = textureAsset,
                                              .asset = &asset,
                                              .representation =
                                                  AssetRepresentation::GPU });
                if (asset) {
                    texHandle = as<Texture2D>(asset)->GPUHandle;
                }
                triedToGetTexture = true;
            }
            char const* textypenames[] = { "Texture2D", "Cubemap" };
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::Combo("###Texture Type", (int*)&textype, textypenames, 2);
            if (texHandle && texHandle.nativeHandle()) {
                float2 pos = ImGui::GetCursorScreenPos();
                float2 size = ImGui::GetContentRegionAvail();
                auto* DL = ImGui::GetWindowDrawList();
                DL->AddImageRounded(texHandle.nativeHandle(), pos, pos + size,
                                    { 0, 0 }, { 1, 1 }, IM_COL32_WHITE, 5);
            }
            break;
        }
        }
    }

    DataType dataType;
    float4 value = .5;
    AssetHandle textureAsset;
    TextureHandle texHandle;
    TextureSubType textype = TextureSubType::Texture2D;
    bool triedToGetTexture = false;
};

/// Any fragment shader input, i.e., rasterizer data
struct InputNodeController: MaterialNodeController {
    static NodeDesc Make(float2 position, InputKind kind) {
        return { .name = std::string(magic_enum::enum_name(kind)),
                 .position = position,
                 .minSize = { 100, 0 },
                 .controller = std::make_unique<InputNodeController>(kind) };
    }

    InputNodeController(InputKind kind): kind(kind) {}

    explicit InputNodeController(YAML::Node const& root) {
        kind = root["Kind"].as<InputKind>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = NodeType::Input;
        root["Kind"] = kind;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override { return {}; }

    std::vector<PinDesc> makeOutputDescriptors() override {
        switch (kind) {
        case InputKind::TextureCoords:
            return {
                { .name = "Coords", .userType = (PinUserType)DataType::Float2 },
                { .name = "U",
                  .userType = (PinUserType)DataType::Float,
                  .color = PinChannelColors[0] },
                { .name = "V",
                  .userType = (PinUserType)DataType::Float,
                  .color = PinChannelColors[1] },
            };

        case InputKind::Normals:
            [[fallthrough]];
        case InputKind::Tangents:
            [[fallthrough]];
        case InputKind::Binormals:
            [[fallthrough]];
        case InputKind::WorldPosition:
            return { { .name = "Vector",
                       .userType = (PinUserType)DataType::Float3 } };
        }
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& g) const override {
        switch (kind) {
        case InputKind::TextureCoords: {
            auto* coords =
                ctx.rasterizerInput(RasterizerInputKind::TextureCoords);
            return { coords,
                     g.addValue(allocate<VectorElement>(ctx, 0, coords)),
                     g.addValue(allocate<VectorElement>(ctx, 1, coords)) };
        }
        case InputKind::Normals:
            return { ctx.rasterizerInput(RasterizerInputKind::VertexNormals) };
        case InputKind::Tangents:;
            return { ctx.rasterizerInput(RasterizerInputKind::VertexTangents) };
        case InputKind::Binormals:
            return { ctx.rasterizerInput(
                RasterizerInputKind::VertexBinormals) };
        case InputKind::WorldPosition:
            return { ctx.rasterizerInput(RasterizerInputKind::WorldPosition) };
        }
    }

    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    InputKind kind;
};

struct BuiltinFunctionController: OverloadableNodeController {
    static size_t numInputs(FunctionType type) {
        switch (type) {
        case FunctionType::Normalize:
            [[fallthrough]];
        case FunctionType::Sqrt:
            return 1;
        }
    }

    static NodeDesc Make(float2 position, FunctionType type) {
        return { .name = std::string(magic_enum::enum_name(type)),
                 .position = position,
                 .controller =
                     std::make_unique<BuiltinFunctionController>(type) };
    }

    BuiltinFunctionController(FunctionType type):
        OverloadableNodeController(numInputs(type), DataType::AnyVector),
        type(type) {}

    explicit BuiltinFunctionController(YAML::Node const& root):
        OverloadableNodeController(root) {
        type = root["FunctionType"].as<FunctionType>();
        inputTypes = root["InputTypes"].as<std::vector<DataType>>();
    }

    YAML::Node serialize() const override {
        YAML::Node root = OverloadableNodeController::serialize();
        root["Type"] = NodeType::BuiltinFunction;
        root["FunctionType"] = type;
        root["InputTypes"] = inputTypes;
        return root;
    }

    float2 preferredContentSize() const override { return { 70, 0 }; }

    std::vector<PinDesc> makeInputDescriptors() override {
        switch (type) {
        case FunctionType::Normalize:
        case FunctionType::Sqrt:
            return {
                { .name = "Input",
                  .userType = (PinUserType)DataType::AnyVector },
            };
        }
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        switch (type) {
        case FunctionType::Normalize:
        case FunctionType::Sqrt:
            return {
                { .name = "Result", .userType = (PinUserType)inputTypes[0] },
            };
        }
    }

    static BuiltinFunctionKind translateFunctionType(FunctionType type) {
        switch (type) {
        case FunctionType::Normalize:
            return BuiltinFunctionKind::Normalize;
        case FunctionType::Sqrt:
            return BuiltinFunctionKind::Sqrt;
        }
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& g) const override {
        if (isAnyInputUntyped()) {
            return {};
        }
        using enum BuiltinFunctionKind;
        switch (type) {
        case FunctionType::Normalize:
            [[fallthrough]];
        case FunctionType::Sqrt:
            auto* inType = translateType(ctx, inputTypes[0]);
            auto* value =
                g.addValue<BuiltinFunction>(ctx, translateFunctionType(type),
                                            std::array{ inType });
            return { value };
        }
    }

    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    FunctionType type;
};

struct AppendNodeController: MaterialNodeController {
    static NodeDesc Make(float2 position, int numComponents) {
        return { .name = "Append",
                 .position = position,
                 .minSize = { 50, 0 },
                 .controller =
                     std::make_unique<AppendNodeController>(numComponents) };
    }

    AppendNodeController(int numComponents): numComponents(numComponents) {}

    explicit AppendNodeController(YAML::Node const& node) {
        numComponents = node["NumComponents"].as<int>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = NodeType::Append;
        root["NumComponents"] = numComponents;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return iota(0, numComponents) | transform([](int index) {
            return PinDesc{ .name = std::array{ "Red", "Green", "Blue",
                                                "Alpha" }[index],
                            .userType = (PinUserType)DataType::Float,
                            .flags = PinFlags::Optional };
        }) | ranges::to<std::vector>;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return {
            { .name = "",
              .userType =
                  (PinUserType)((int)DataType::Float + numComponents - 1) },
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        return { graph.addValue(
            allocate<VectorBuilder>(ctx.vectorType(numComponents),
                                    numComponents)) };
    }

    int numComponents;
};

struct SplitNodeController: MaterialNodeController {
    static NodeDesc Make(float2 position, int numComponents) {
        return { .name = "Split",
                 .position = position,
                 .minSize = { 50, 0 },
                 .controller =
                     std::make_unique<SplitNodeController>(numComponents) };
    }

    SplitNodeController(int numComponents): numComponents(numComponents) {}

    explicit SplitNodeController(YAML::Node const& node) {
        numComponents = node["NumComponents"].as<int>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = NodeType::Split;
        root["NumComponents"] = numComponents;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return {
            { .name = "",
              .userType =
                  (PinUserType)((int)DataType::Float + numComponents - 1) },
        };
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return std::vector<PinDesc>(numComponents,
                                    PinDesc{ .name = "",
                                             .userType = (PinUserType)
                                                 DataType::Float });
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        return iota(0, numComponents) | transform([&](int i) {
            return graph.addValue(allocate<VectorElement>(ctx, i, nullptr));
        }) | ToSmallVector<>;
    }

    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return false; }

    int numComponents;
};

struct ArithmeticNodeController: OverloadableNodeController {
    static NodeDesc Make(ArithmeticOp op, float2 position) {
        return { .name = std::string(magic_enum::enum_name(op)),
                 .position = position,
                 .minSize = { 50, 0 },
                 .controller = std::make_unique<ArithmeticNodeController>(op) };
    }

    ArithmeticNodeController(ArithmeticOp op):
        OverloadableNodeController(2, DataType::AnyVector), operation(op) {}

    explicit ArithmeticNodeController(YAML::Node const& root):
        OverloadableNodeController(root) {
        operation = root["Operation"].as<ArithmeticOp>();
    }

    YAML::Node serialize() const override {
        YAML::Node root = OverloadableNodeController::serialize();
        root["Type"] = NodeType::Arithmetic;
        root["Operation"] = operation;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return { { .name = "A",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::AnyVector },
                 { .name = "B",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::AnyVector } };
    }

    DataType selectOutputType() const {
        using enum DataType;
        auto a = inputTypes[0];
        auto b = inputTypes[1];
        if (a == Float || a == AnyVector) {
            return b;
        }
        if (b == Float || b == AnyVector) {
            return a;
        }
        return a;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return { { .name = "Result",
                   .userType = (PinUserType)selectOutputType() } };
    }

    static ArithmeticOperation::Kind translateOp(ArithmeticOp op) {
        switch (op) {
        case ArithmeticOp::Add:
            return ArithmeticOperation::Add;
        case ArithmeticOp::Sub:
            return ArithmeticOperation::Sub;
        case ArithmeticOp::Mul:
            return ArithmeticOperation::Mul;
        case ArithmeticOp::Div:
            return ArithmeticOperation::Div;
        }
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        if (isAnyInputUntyped()) {
            return {};
        }
        auto* value =
            graph.addValue<ArithmeticOperation>(translateOp(operation),
                                                translateType(ctx,
                                                              inputTypes[0]),
                                                translateType(ctx,
                                                              inputTypes[1]));
        return { value };
    }

    bool alwaysDisplayInputNames() const override { return true; }

    bool alwaysDisplayOutputNames() const override { return false; }

    ArithmeticOp operation;
};

struct TextureSamplerController: MaterialNodeController {
    static NodeDesc Make(TextureSubType subtype, float2 position) {
        return { .name = "Texture Sample",
                 .position = position,
                 .minSize = { 100, 0 },
                 .controller =
                     std::make_unique<TextureSamplerController>(subtype) };
    }

    explicit TextureSamplerController(TextureSubType subtype):
        subtype(subtype) {}

    explicit TextureSamplerController(YAML::Node const& node) {
        mode = node["Mode"].as<SampleMode>();
        subtype = node["Subtype"].as<TextureSubType>();
    }

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = NodeType::TextureSampler;
        node["Subtype"] = subtype;
        node["Mode"] = mode;
        return node;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return { { .name = "Color", .userType = (PinUserType)DataType::Float3 },
                 { .name = "Red",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = PinChannelColors[0] },
                 { .name = "Green",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = PinChannelColors[1] },
                 { .name = "Blue",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = PinChannelColors[2] },
                 { .name = "Alpha",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float } };
    }

    static DataType getUVType(TextureSubType subtype) {
        switch (subtype) {
        case TextureSubType::Texture2D:
            return DataType::Float2;
        case TextureSubType::TextureCube:
            return DataType::Float3;
        }
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return {
            { .name = "Texture",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Texture },
            { .name = "UVs",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)getUVType(subtype) },
        };
    }

    void bodyContent() override {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SetCursorPos({ 0, 40 });
        char const* items[2] = { "Color", "Normal" };
        ImGui::Combo("###Mode", (int*)&mode, items, 2);
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        auto* sample = graph.addValue(
            allocate<TextureSampler>(ctx, nullptr, nullptr, mode));
        auto* r = graph.addValue(allocate<VectorElement>(ctx, 0, sample));
        auto* g = graph.addValue(allocate<VectorElement>(ctx, 1, sample));
        auto* b = graph.addValue(allocate<VectorElement>(ctx, 2, sample));
        auto* a = graph.addValue(allocate<VectorElement>(ctx, 3, sample));
        auto* rgb = graph.addValue(
            allocate<VectorBuilder>(ctx, std::vector<Value*>{ r, g, b }));
        return {
            rgb,   r, g, b, a,
            sample // This is hacky but we put sample here so it will be linked,
                   // even though there is no corresponding output pin
        };
    }

    vml::float4 nodeColor() override { return { 0.2, 0.4, 1, 1 }; }

    SampleMode mode = SampleMode::Color;
    TextureSubType subtype;
};

static std::unique_ptr<NodeController> deserializeNodeController(
    NodeType type, YAML::Node const& node) {
    switch (type) {
    case NodeType::Output:
        return std::make_unique<OutputNodeController>(node);
    case NodeType::Input:
        return std::make_unique<InputNodeController>(node);
    case NodeType::Constant:
        return std::make_unique<ConstantController>(node);
    case NodeType::Append:
        return std::make_unique<AppendNodeController>(node);
    case NodeType::Split:
        return std::make_unique<SplitNodeController>(node);
    case NodeType::BuiltinFunction:
        return std::make_unique<BuiltinFunctionController>(node);
    case NodeType::Arithmetic:
        return std::make_unique<ArithmeticNodeController>(node);
    case NodeType::TextureSampler:
        return std::make_unique<TextureSamplerController>(node);
    }
}

struct MaterialModelImpl: bloom::MaterialModel {
    bool hasCPURepr() const override { return !editorGraph.empty(); }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["ShadingModel"] = shadingModel;
        root["NodeGraph"] = editorGraph.serialize();
        return root;
    }

    void deserialize(YAML::Node const& node) override {
        shadingModel = node["ShadingModel"].as<MaterialShadingModel>();
        editorGraph.deserialize(node["NodeGraph"], [](YAML::Node const& node) {
            return deserializeNodeController(node["Type"].as<NodeType>(), node);
        });
    }

    static MaterialNodeController* getController(Node* node) {
        return dynamic_cast<MaterialNodeController*>(node->controller());
    }

    MaterialSourceData generateSource() const override {
        Context ctx;
        MaterialGraph graph(shadingModel);
        MaterialNodeController::ValueMap map;
        for (auto* node: editorGraph.nodes()) {
            auto* controller = getController(node);
            map[node] = controller->createGraphNodes(ctx, graph);
        }
        for (auto* node: editorGraph.nodes()) {
            getController(node)->linkGraphNode(ctx, map);
        }
        switch (shadingModel) {
        case MaterialShadingModel::Unlit:
            if (auto* root = graph.root()) {
                auto* newRoot = graph.addValue<AggregateElement>(root, 1);
                graph.setRoot(newRoot);
            }
            break;
        case MaterialShadingModel::DefaultLit:
            break;
        }
        auto codegen = makeCodeGenerator(ctx, graph, {});
        auto result = codegen->run();
        return {
            .source = std::move(result.source),
            .fragmentTextures = std::move(result.fragmentTextures),
        };
    }

    void setShadingModel(MaterialShadingModel model) {
        if (shadingModel == model) {
            return;
        }
        shadingModel = model;
    }

    mutable NodeEditorGraph editorGraph;
    MaterialShadingModel shadingModel;
};

} // namespace

static MaterialModelImpl* getModel(Material* material) {
    if (!material) {
        return nullptr;
    }
    return dynamic_cast<MaterialModelImpl*>(material->model.get());
}

std::unique_ptr<bloom::MaterialModel> bloom::makeMaterialModel() {
    return std::make_unique<MaterialModelImpl>();
}

namespace {

struct MaterialEditor: View {
    MaterialEditor();

    void display() override;
    void displayOptions();
    void onInput(InputEvent& event) override;
    YAML::Node serialize() const override;
    void deserialize(YAML::Node node) override;
    void doOpenData(std::any data) override {
        if (auto* mat = std::any_cast<Reference<Material>>(&data)) {
            material = *mat;
            nodeEditor.setModel(&getModel(material.get())->editorGraph);
        }
    }
    void compile();

    Reference<Material> material;
    NodeEditor nodeEditor;
    SplitView splitView;
    bool showOptions = false;
};

struct Delegate: NodeEditorDelegate {
    Delegate(MaterialEditor& editor): editor(editor) {}

    void backgroundPopupMenu(vml::float2 position) override {
        auto* editorGraph = &getModel()->editorGraph;
        if (!editorGraph) {
            return;
        }
        if (ImGui::MenuItem("Output")) {
            editorGraph->addNode(OutputNodeController::Make(position));
        }
        if (ImGui::BeginMenu("Inputs")) {
            for (auto kind: magic_enum::enum_values<InputKind>()) {
                if (ImGui::MenuItem(
                        std::string(magic_enum::enum_name(kind)).c_str()))
                {
                    editorGraph->addNode(
                        InputNodeController::Make(position, kind));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Append")) {
            for (int i = 2; i <= 4; ++i) {
                if (ImGui::MenuItem(utl::strcat("Float", i).c_str())) {
                    editorGraph->addNode(
                        AppendNodeController::Make(position, i));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Split")) {
            for (int i = 2; i <= 4; ++i) {
                if (ImGui::MenuItem(utl::strcat("Float", i).c_str())) {
                    editorGraph->addNode(
                        SplitNodeController::Make(position, i));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Functions")) {
            for (auto type: magic_enum::enum_values<FunctionType>()) {
                if (ImGui::MenuItem(
                        std::string(magic_enum::enum_name(type)).c_str()))
                {
                    editorGraph->addNode(
                        BuiltinFunctionController::Make(position, type));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Constant")) {
            for (auto type: magic_enum::enum_values<DataType>()) {
                std::string name(magic_enum::enum_name(type));
                if (type != DataType::AnyVector &&
                    ImGui::MenuItem(name.c_str()))
                {
                    editorGraph->addNode(
                        ConstantController::Make(position, type));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Texture Sample")) {
            editorGraph->addNode(
                TextureSamplerController::Make(TextureSubType::Texture2D,
                                               position));
        }
        if (ImGui::MenuItem("Cube Texture Sample")) {
            editorGraph->addNode(
                TextureSamplerController::Make(TextureSubType::TextureCube,
                                               position));
        }
        for (size_t i = 0; i < EnumCount<ArithmeticOp>; ++i) {
            auto op = (ArithmeticOp)i;
            if (ImGui::MenuItem(std::string(magic_enum::enum_name(op)).c_str()))
            {
                editorGraph->addNode(
                    ArithmeticNodeController::Make(op, position));
            }
        }
    }

    bool isPinTypeCompatible(PinUserType input,
                             PinUserType output) const override {
        auto in = (DataType)input;
        auto out = (DataType)output;
        if (in == out) {
            return true;
        }
        if ((int)in >= (int)DataType::Float &&
            (int)in <= (int)DataType::Float4 && out == DataType::AnyVector)
        {
            return true;
        }
        if ((int)out >= (int)DataType::Float &&
            (int)out <= (int)DataType::Float4 && in == DataType::AnyVector)
        {
            return true;
        }
        return false;
    }

    MaterialModelImpl* getModel() { return ::getModel(editor.material.get()); }

    MaterialEditor& editor;
};

} // namespace

POPPY_REGISTER_VIEW(MaterialEditor, "Material Editor");

MaterialEditor::MaterialEditor(): nodeEditor(commandPaletteWrite()) {
    auto& cmd = commandPaletteWrite();
    auto* compileCmd = cmd.add("Compile", { Key::B, ModFlags::Control },
                               [this] { compile(); },
                               [this]() -> bool { return material.get(); });
    // clang-format off
    setToolbar({
        ToolbarIconButton(compileCmd, "ok"),
        ToolbarSpacer(),
        ToolbarTextItem()
            .text([this] { return material ? material->name().c_str() : ""; })
            .font(FontDesc::UIDefault().setWeight(FontWeight::Bold)),
        ToolbarSpacer(),
        ToolbarDropdownMenu()
            .icon("plus")
            .content([this] {
                auto* del = nodeEditor.getDelegate();
                if (!del) return;
                del->backgroundPopupMenu({ 0, 0 });
            })
            .enabled([this] { return nodeEditor.getDelegate(); }),
        ToolbarIconButton("cog-alt")
            .onClick([this] { showOptions ^= true; })
            .enabled([this] { return material != nullptr; })
    }); // clang-format on
    setPadding({ 0, 0 });
    nodeEditor.setDelegate(std::make_unique<Delegate>(*this));
}

void MaterialEditor::display() {
    gEditor = &editor();
    if (!material) {
        displayEmptyWithReason("No Material Open");
        return;
    }
    if (showOptions) {
        // clang-format off
        splitView.display({
            [this] { nodeEditor.display(); },
            [this] { displayOptions(); }
        }, SplitView::NoPadding); // clang-format on
    }
    else {
        nodeEditor.display();
    }
}

void MaterialEditor::displayOptions() {
    ImGui::BeginChild("PropertiesPanel", {},
                      ImGuiChildFlags_AlwaysUseWindowPadding);
    using namespace propertiesView;
    auto* model = getModel(material.get());
    BL_ASSERT(model);
    beginSection("Properties");
    beginProperty("Shading Model");
    auto currentModel = model->shadingModel;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("###Shading Model",
                          std::string(magic_enum::enum_name(currentModel))
                              .c_str()))
    {
        for (auto value: magic_enum::enum_values<MaterialShadingModel>()) {
            bool selected = value == currentModel;
            if (ImGui::Selectable(std::string(magic_enum::enum_name(value))
                                      .c_str(),
                                  selected) &&
                !selected)
            {
                model->setShadingModel(value);
            }
        }
        ImGui::EndCombo();
    }
    endSection();
    ImGui::EndChild();
}

void MaterialEditor::onInput(InputEvent& event) { nodeEditor.onInput(event); }

YAML::Node MaterialEditor::serialize() const {
    YAML::Node root;
    root["NodeEditor"] = nodeEditor.serialize();
    return root;
}

void MaterialEditor::deserialize(YAML::Node node) {
    nodeEditor.deserialize(node["NodeEditor"]);
}

void MaterialEditor::compile() {
    BL_ASSERT(material.get());
    dispatch(DispatchToken::NextFrame, [this] {
        editor()
            .coreSystems()
            .assetManager()
            .makeAvailable(material->handle(), AssetRepresentation::GPU,
                           /* forceReload: */ true);
    });
}
