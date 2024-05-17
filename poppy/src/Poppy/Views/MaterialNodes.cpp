#include <imgui.h>
#include <utl/function_view.hpp>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/Serialization.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/MaterialNodeController.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;
using namespace matc;
using namespace nodeEditor;

static constexpr std::array<float4, 3> ColorChannelColors = {
    { { 1, 0, 0, 1 }, { .4, .9, .3, 1 }, { 0, 0.5, 1, 1 } }
};

static constexpr std::array<char const*, 4> ColorChannelNames = {
    "Red", "Green", "Blue", "Alpha"
};

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

namespace {

template <typename E, typename Controller>
static utl::function_view<std::unique_ptr<Node>(E)> defaultControllerFactory() {
    return [](E e) { return std::make_unique<Node>(Controller::Make(e)); };
}

template <typename E, typename Controller = void>
static std::unique_ptr<Node> enumMenu(
    char const* menuName,
    utl::function_view<std::unique_ptr<Node>(E)> make =
        defaultControllerFactory<E, Controller>(),
    utl::function_view<std::string(E)> itemName = {},
    utl::function_view<bool(E)> filter = {}) {
    if (!itemName) {
        itemName = [](E e) { return std::string(magic_enum::enum_name(e)); };
    }
    if (!filter) {
        filter = [](auto) { return true; };
    }
    if (!ImGui::BeginMenu(menuName)) {
        return nullptr;
    }
    std::unique_ptr<Node> node;
    for (auto e: magic_enum::enum_values<E>()) {
        if (!filter(e) || !ImGui::MenuItem(itemName(e).c_str())) {
            continue;
        }
        node = make(e);
    }
    ImGui::EndMenu();
    return node;
}

struct OutputController: MaterialNodeController {
    static constexpr auto Name = "OutputController";

    static NodeDesc Make() {
        return { .name = "Output",
                 .controller = std::make_unique<OutputController>() };
    }

    OutputController() = default;

    explicit OutputController(YAML::Node const&) {}

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = Name;
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

    Value* defaultInputValue(Context& ctx, size_t index) const override {
        switch (index) {
        case 0: // Base color
            return ctx.float3Constant({ 0, 0, 0 });
        case 1: // Emissive
            return ctx.float3Constant({ 0, 0, 0 });
        case 2: // Roughness
            return ctx.floatConstant(1);
        case 3: // Metallic
            return ctx.floatConstant(0);
        case 4: // AO
            return ctx.floatConstant(1);
        case 5: // Normal
            return ctx.float3Constant({ 0, 0, 1 });
        default:
            BL_UNREACHABLE();
        }
    }

    vml::float4 nodeColor() override { return { 0.1, 0.1, 0.1, 1 }; }
};

REGISTER_MATERIAL_NODE(OutputController, {
    if (ImGui::MenuItem("Output")) {
        return std::make_unique<Node>(OutputController::Make());
    }
})

enum class InputKind {
    TextureCoords,
    Normals,
    Tangents,
    Binormals,
    WorldPosition
};

/// Any fragment shader input, i.e., rasterizer data
struct InputController: MaterialNodeController {
    static constexpr auto Name = "InputController";

    static NodeDesc Make(InputKind kind) {
        return { .name = std::string(magic_enum::enum_name(kind)),
                 .controller = std::make_unique<InputController>(kind) };
    }

    explicit InputController(InputKind kind): kind(kind) {}

    explicit InputController(YAML::Node const& root) {
        kind = root["Kind"].as<InputKind>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = Name;
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
                  .color = ColorChannelColors[0] },
                { .name = "V",
                  .userType = (PinUserType)DataType::Float,
                  .color = ColorChannelColors[1] },
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

    float2 preferredContentSize() const override { return { 70, 0 }; }

    float4 nodeColor() override { return { 1, 0.3, 0.1, 1 }; }

    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    InputKind kind;
};

REGISTER_MATERIAL_NODE(InputController, {
    return enumMenu<InputKind, InputController>("Inputs");
})

struct ConstantController: MaterialNodeController {
    static constexpr auto Name = "ConstantController";

    static NodeDesc Make(DataType dataType) {
        using enum DataType;
        NodeDesc desc = {
            .name = utl::strcat(magic_enum::enum_name(dataType), " Constant"),
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
        node["Type"] = Name;
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
                { .name = ColorChannelNames[0],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[0] },
                { .name = ColorChannelNames[1],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[1] },
            };
        case Float3:
            return {
                { .name = "Value", .userType = (PinUserType)Float3 },
                { .name = ColorChannelNames[0],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[0] },
                { .name = ColorChannelNames[2],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[1] },
                { .name = ColorChannelNames[3],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[2] },
            };
        case Float4:
            return {
                { .name = "Value", .userType = (PinUserType)Float3 },
                { .name = ColorChannelNames[0],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[0] },
                { .name = ColorChannelNames[1],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[1] },
                { .name = ColorChannelNames[2],
                  .userType = (PinUserType)Float,
                  .color = ColorChannelColors[2] },
                { .name = ColorChannelNames[3],
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
#if 0
                gEditor->dispatch(DispatchToken::Now,
                                  AMGetAsset{ .handle = textureAsset,
                                              .asset = &asset,
                                              .representation =
                                                  AssetRepresentation::GPU });
#endif
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

REGISTER_MATERIAL_NODE(ConstantController, {
    return enumMenu<DataType>(
        "Constants", defaultControllerFactory<DataType, ConstantController>(),
        {}, [](DataType t) { return t != DataType::AnyVector; });
})

enum class FunctionType { Normalize, Sqrt };

struct BuiltinFunctionController: OverloadableNodeController {
    static constexpr auto Name = "BuiltinFunction";

    static size_t numInputs(FunctionType type) {
        switch (type) {
        case FunctionType::Normalize:
            [[fallthrough]];
        case FunctionType::Sqrt:
            return 1;
        }
    }

    static NodeDesc Make(FunctionType type) {
        return { .name = std::string(magic_enum::enum_name(type)),
                 .controller =
                     std::make_unique<BuiltinFunctionController>(type) };
    }

    explicit BuiltinFunctionController(FunctionType type):
        OverloadableNodeController(numInputs(type), DataType::AnyVector),
        type(type) {}

    explicit BuiltinFunctionController(YAML::Node const& root):
        OverloadableNodeController(root) {
        type = root["FunctionType"].as<FunctionType>();
    }

    YAML::Node serialize() const override {
        YAML::Node root = OverloadableNodeController::serialize();
        root["Type"] = Name;
        root["FunctionType"] = type;
        return root;
    }

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
                { .name = "Result", .userType = (PinUserType)inputType(0) },
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
            auto* inType = translateType(ctx, inputType(0));
            auto* value =
                g.addValue<BuiltinFunction>(ctx, translateFunctionType(type),
                                            std::array{ inType });
            return { value };
        }
    }

    float2 preferredContentSize() const override { return { 70, 0 }; }
    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    FunctionType type;
};

REGISTER_MATERIAL_NODE(BuiltinFunctionController, {
    return enumMenu<FunctionType, BuiltinFunctionController>("Functions");
})

struct AppendController: MaterialNodeController {
    static constexpr auto Name = "AppendController";

    static NodeDesc Make(int numComponents) {
        return { .name = "Append",
                 .controller =
                     std::make_unique<AppendController>(numComponents) };
    }

    explicit AppendController(int numComponents):
        numComponents(numComponents) {}

    explicit AppendController(YAML::Node const& node) {
        numComponents = node["NumComponents"].as<int>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = Name;
        root["NumComponents"] = numComponents;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return iota(0, numComponents) | transform([](int index) {
            return PinDesc{ .name = ColorChannelNames[index],
                            .userType = (PinUserType)DataType::Float,
                            .flags = PinFlags::Optional };
        }) | ranges::to<std::vector>;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return {
            { .userType = (PinUserType)FloatNType(numComponents) },
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        return { graph.addValue(
            allocate<VectorBuilder>(ctx.vectorType(numComponents),
                                    numComponents)) };
    }

    float2 preferredContentSize() const override { return { 50, 0 }; }

    int numComponents;
};

REGISTER_MATERIAL_NODE(AppendController, {
    std::unique_ptr<Node> result;
    if (ImGui::BeginMenu("Append")) {
        for (int i = 2; i <= 4; ++i) {
            if (ImGui::MenuItem(utl::strcat("Float", i).c_str())) {
                result = std::make_unique<Node>(AppendController::Make(i));
            }
        }
        ImGui::EndMenu();
    }
    return result;
})

struct SplitController: MaterialNodeController {
    static constexpr auto Name = "SplitController";

    static NodeDesc Make(int numComponents) {
        return { .name = "Split",
                 .controller =
                     std::make_unique<SplitController>(numComponents) };
    }

    explicit SplitController(int numComponents): numComponents(numComponents) {}

    explicit SplitController(YAML::Node const& node) {
        numComponents = node["NumComponents"].as<int>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = Name;
        root["NumComponents"] = numComponents;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return {
            { .userType =
                  (PinUserType)((int)DataType::Float + numComponents - 1) },
        };
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return std::vector<PinDesc>(numComponents,
                                    { .userType =
                                          (PinUserType)DataType::Float });
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        return iota(0, numComponents) | transform([&](int i) {
            return graph.addValue(allocate<VectorElement>(ctx, i, nullptr));
        }) | ToSmallVector<>;
    }

    float2 preferredContentSize() const override { return { 50, 0 }; }
    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return false; }

    int numComponents;
};

REGISTER_MATERIAL_NODE(AppendController, {
    std::unique_ptr<Node> result;
    if (ImGui::BeginMenu("Split")) {
        for (int i = 2; i <= 4; ++i) {
            if (ImGui::MenuItem(utl::strcat("Float", i).c_str())) {
                result = std::make_unique<Node>(SplitController::Make(i));
            }
        }
        ImGui::EndMenu();
    }
    return result;
})

struct ArithmeticController: OverloadableNodeController {
    static constexpr auto Name = "ArithmeticController";

    static NodeDesc Make(ArithmeticOperation::Kind op) {
        return { .name = std::string(magic_enum::enum_name(op)),
                 .controller = std::make_unique<ArithmeticController>(op) };
    }

    ArithmeticController(ArithmeticOperation::Kind op):
        OverloadableNodeController(2, DataType::AnyVector), operation(op) {}

    explicit ArithmeticController(YAML::Node const& root):
        OverloadableNodeController(root) {
        operation = root["Operation"].as<ArithmeticOperation::Kind>();
    }

    YAML::Node serialize() const override {
        YAML::Node root = OverloadableNodeController::serialize();
        root["Type"] = Name;
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
        auto a = inputType(0);
        auto b = inputType(1);
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

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        if (isAnyInputUntyped()) {
            return {};
        }
        auto* value =
            graph.addValue<ArithmeticOperation>(operation,
                                                translateType(ctx,
                                                              inputType(0)),
                                                translateType(ctx,
                                                              inputType(1)));
        return { value };
    }

    float2 preferredContentSize() const override { return { 50, 0 }; }
    bool alwaysDisplayInputNames() const override { return true; }
    bool alwaysDisplayOutputNames() const override { return false; }

    ArithmeticOperation::Kind operation;
};

REGISTER_MATERIAL_NODE(ArithmeticController, {
    return enumMenu<ArithmeticOperation::Kind, ArithmeticController>(
        "Arithmetic");
})

struct TextureSamplerController: MaterialNodeController {
    static constexpr auto Name = "TextureSamplerController";

    static NodeDesc Make(TextureSubType subtype) {
        return { .name = "Texture Sample",
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
        node["Type"] = Name;
        node["Subtype"] = subtype;
        node["Mode"] = mode;
        return node;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return { { .name = "Color", .userType = (PinUserType)DataType::Float3 },
                 { .name = ColorChannelNames[0],
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = ColorChannelColors[0] },
                 { .name = ColorChannelNames[1],
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = ColorChannelColors[1] },
                 { .name = ColorChannelNames[2],
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = ColorChannelColors[2] },
                 { .name = ColorChannelNames[3],
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
            { .name = "Texture", .userType = (PinUserType)DataType::Texture },
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

    Value* defaultInputValue(Context& ctx, size_t index) const override {
        switch (index) {
        case 1: // UVs
            return ctx.rasterizerInput(RasterizerInputKind::TextureCoords);
        default:
            BL_UNREACHABLE();
        }
    }

    vml::float4 nodeColor() override { return { 0.2, 0.4, 1, 1 }; }
    float2 preferredContentSize() const override { return { 100, 0 }; }

    SampleMode mode = SampleMode::Color;
    TextureSubType subtype;
};

REGISTER_MATERIAL_NODE(TextureSamplerController, {
    if (ImGui::MenuItem("Texture Sample")) {
        return std::make_unique<Node>(
            TextureSamplerController::Make(TextureSubType::Texture2D));
    }
    if (ImGui::MenuItem("Cube Texture Sample")) {
        return std::make_unique<Node>(
            TextureSamplerController::Make(TextureSubType::TextureCube));
    }
})

} // namespace
