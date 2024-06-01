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

static constexpr DataTypeMask FloatN = DataTypeMask::FloatN;
static constexpr DataTypeMask VectorN = DataTypeMask::VectorN;

static constexpr std::array<float4, 3> ColorChannelColors = {
    { { 1, 0, 0, 1 }, { .4, .9, .3, 1 }, { 0, 0.5, 1, 1 } }
};

static constexpr std::array<char const*, 4> ColorChannelNames = {
    "Red", "Green", "Blue", "Alpha"
};

static DataTypeMask mask(auto... t) { return { t... }; }

namespace {

struct Colors {
    float4 output = { 0.1, 0.1, 0.1, 1 };
    float4 input = { 1, 0, 0.5, 1 };
    float4 constant = { 0, 0.55, 0.066, 1 };
    float4 parameter = { 0.89, 0, 1, 1 };
    float4 sampler = { 0, 0.45, 1, 1 };
    float4 util = { 0, 0.86, 0.14, 1 };
};

} // namespace

static Colors colors;

namespace poppy {

void displayMaterialNodeColorEditor() {
    ImGui::Begin("Material Node Colors");
#define EDIT(Name) ImGui::ColorEdit4(#Name, colors.Name.data())
    EDIT(output);
    EDIT(input);
    EDIT(constant);
    EDIT(parameter);
    EDIT(sampler);
    EDIT(util);
#undef EDIT
    ImGui::End();
}

} // namespace poppy

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
        return ctx.vectorType(numComponents(dataType));
    }
    case Texture:
        BL_UNIMPLEMENTED();
    }
}

namespace {

template <typename E, typename Controller>
static utl::function_view<std::unique_ptr<Node>(E)> defaultControllerFactory() {
    return [](E e) { return std::make_unique<Node>(Controller::Make(e)); };
}

template <typename Controller = void, ranges::range R,
          typename E = ranges::range_value_type_t<R>>
static std::unique_ptr<Node> enumMenu(
    R range, char const* menuName,
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
    for (auto e: range) {
        if (!filter(e) || !ImGui::MenuItem(itemName(e).c_str())) {
            continue;
        }
        node = make(e);
    }
    ImGui::EndMenu();
    return node;
}

template <typename E, typename Controller = void>
static std::unique_ptr<Node> enumMenu(
    char const* menuName,
    utl::function_view<std::unique_ptr<Node>(E)> make =
        defaultControllerFactory<E, Controller>(),
    utl::function_view<std::string(E)> itemName = {},
    utl::function_view<bool(E)> filter = {}) {
    return enumMenu(magic_enum::enum_values<E>(), menuName, make, itemName,
                    filter);
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
              .userType = mask(DataType::Float3) },
            { .name = "Emissive",
              .flags = PinFlags::Optional,
              .userType = mask(DataType::Float3) },
            { .name = "Roughness",
              .flags = PinFlags::Optional,
              .userType = mask(DataType::Float) },
            { .name = "Metallic",
              .flags = PinFlags::Optional,
              .userType = mask(DataType::Float) },
            { .name = "Ambient Occlusion",
              .flags = PinFlags::Optional,
              .userType = mask(DataType::Float) },
            { .name = "Normal",
              .flags = PinFlags::Optional,
              .userType = mask(DataType::Float3) },
            { .name = "Transparency",
              .flags = PinFlags::Optional,
              .userType = mask(DataType::Float) },
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
            ctx.floatType(),
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
        case 6: // Normal
            return ctx.floatConstant(1);
        default:
            BL_UNREACHABLE();
        }
    }

    float4 nodeColor() override { return colors.output; }
};

REGISTER_MATERIAL_NODE(OutputController, {
    if (ImGui::MenuItem("Output")) {
        return std::make_unique<Node>(OutputController::Make());
    }
})

/// Any fragment shader input, i.e., rasterizer data
struct InputController: MaterialNodeController {
    static constexpr auto Name = "InputController";

    static NodeDesc Make(RasterizerInputKind kind) {
        return { .name = std::string(magic_enum::enum_name(kind)),
                 .controller = std::make_unique<InputController>(kind) };
    }

    explicit InputController(RasterizerInputKind kind): kind(kind) {}

    explicit InputController(YAML::Node const& root) {
        kind = root["Kind"].as<RasterizerInputKind>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = Name;
        root["Kind"] = kind;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override { return {}; }

    std::vector<PinDesc> makeOutputDescriptors() override {
        using enum RasterizerInputKind;
        switch (kind) {
        case TextureCoords:
            return { { .name = "Coords", .userType = mask(DataType::Float2) } };
        case WorldNormals:
            [[fallthrough]];
        case WorldTangents:
            [[fallthrough]];
        case WorldBinormals:
            [[fallthrough]];
        case WorldPosition:
            [[fallthrough]];
        case ModelNormals:
            return { { .name = "Vector", .userType = mask(DataType::Float3) } };
        }
    }

    utl::small_vector<Value*> createGraphNodes(Context& ctx,
                                               MaterialGraph&) const override {
        return { ctx.rasterizerInput(kind) };
    }

    float2 preferredContentSize() const override { return { 70, 0 }; }

    float4 nodeColor() override { return colors.input; }

    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    RasterizerInputKind kind;
};

REGISTER_MATERIAL_NODE(InputController, {
    return enumMenu<RasterizerInputKind, InputController>("Inputs");
})

struct ConstantController: MaterialNodeController {
    static constexpr auto Name = "ConstantController";

    static std::string nameAsConstant(DataType dataType) {
        return utl::strcat(magic_enum::enum_name(dataType), " Constant");
    }

    static NodeDesc Make(DataType dataType) {
        return { .name = nameAsConstant(dataType),
                 .controller = std::make_unique<ConstantController>(dataType) };
    }

    explicit ConstantController(DataType dataType): dataType(dataType) {}

    explicit ConstantController(YAML::Node const& node) {
        dataType = node["DataType"].as<DataType>();
        value = node["Value"].as<float4>();
        if ((textureHandle = node["Texture"].as<AssetHandle>())) {
            texture = getAsset<Texture2D>(textureHandle);
        }
        textype = node["TextureType"].as<TextureSubType>();
        try {
            parameterName = node["ParameterName"].as<std::string>();
        }
        catch (...) {
        }
        isParameter = node["IsParameter"].as<bool>();
    }

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = Name;
        node["DataType"] = dataType;
        node["Value"] = value;
        node["Texture"] = textureHandle;
        node["TextureType"] = textype;
        node["ParameterName"] = parameterName;
        node["IsParameter"] = isParameter;
        return node;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        using enum DataType;
        switch (dataType) {
        case Float:
            return {
                { .name = "Value", .userType = mask(Float) },
            };
        case Float2:
            return {
                { .name = "Value", .userType = mask(Float2) },
                { .name = ColorChannelNames[0],
                  .userType = mask(Float),
                  .color = ColorChannelColors[0] },
                { .name = ColorChannelNames[1],
                  .userType = mask(Float),
                  .color = ColorChannelColors[1] },
            };
        case Float3:
            return {
                { .name = "Value", .userType = mask(Float3) },
                { .name = ColorChannelNames[0],
                  .userType = mask(Float),
                  .color = ColorChannelColors[0] },
                { .name = ColorChannelNames[2],
                  .userType = mask(Float),
                  .color = ColorChannelColors[1] },
                { .name = ColorChannelNames[3],
                  .userType = mask(Float),
                  .color = ColorChannelColors[2] },
            };
        case Float4:
            return {
                { .name = "Value", .userType = mask(Float3) },
                { .name = ColorChannelNames[0],
                  .userType = mask(Float),
                  .color = ColorChannelColors[0] },
                { .name = ColorChannelNames[1],
                  .userType = mask(Float),
                  .color = ColorChannelColors[1] },
                { .name = ColorChannelNames[2],
                  .userType = mask(Float),
                  .color = ColorChannelColors[2] },
                { .name = ColorChannelNames[3],
                  .userType = mask(Float),
                  .flags = PinFlags::Optional },
            };
        case Texture:
            return {
                { .name = "Texture", .userType = mask(Texture) },
            };
        }
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        using enum DataType;
        switch (dataType) {
        case DataType::Float:
            [[fallthrough]];
        case DataType::Float2:
            [[fallthrough]];
        case DataType::Float3:
            [[fallthrough]];
        case DataType::Float4: {
            if (isParameter) {
                auto* param =
                    graph.addValue<VectorParameter>(ctx,
                                                    numComponents(dataType),
                                                    value, node()->name());
                return { param };
            }
            else {
                return { ctx.vectorConstant(numComponents(dataType), value) };
            }
        }
        case Texture:
            if (isParameter) {
                auto* param =
                    graph.addValue<TextureParameter>(ctx.textureType(textype),
                                                     textureHandle,
                                                     parameterName);
                return { param };
            }
            else {
                return { ctx.textureConstant(textype, textureHandle) };
            }
        }
    }

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
            return { 80, 80 };
        case Float4:
            return { 80, 80 };
        case Texture:
            return float2{ 80, 80 } + float2(0, itemHeight);
        }
    }

    static void colorButton(float4& color, float4& backupColor,
                            DataType dataType) {
        auto flags = ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_HDR;
        bool alpha = dataType == DataType::Float4;
        if (alpha) {
            flags |= ImGuiColorEditFlags_AlphaPreview;
        }
        auto& colVec4 = *(ImVec4*)color.data();
        bool pressed = ImGui::ColorButton("###Button", colVec4, flags,
                                          ImGui::GetContentRegionAvail());
        if (pressed) {
            ImGui::OpenPopup("Picker");
            backupColor = color;
        }
        if (ImGui::BeginPopup("Picker")) {
            ImGui::ColorPicker4("###picker", color.data(),
                                flags | ImGuiColorEditFlags_NoSidePreview |
                                    ImGuiColorEditFlags_NoSmallPreview);
            ImGui::SameLine();
            ImGui::BeginGroup(); // Lock X position
            ImGui::Text("Current");
            auto buttonFlags = ImGuiColorEditFlags_NoPicker |
                               (alpha ? ImGuiColorEditFlags_AlphaPreview : 0);
            ImGui::ColorButton("###current", colVec4, buttonFlags,
                               ImVec2(60, 40));
            ImGui::Text("Previous");
            if (ImGui::ColorButton("##previous", (ImVec4&)backupColor,
                                   buttonFlags, ImVec2(60, 40)))
            {
                color = backupColor;
            }
            ImGui::EndGroup();
            ImGui::EndPopup();
        }
    }

    void bodyContent() override {
        switch (dataType) {
        case DataType::Float:
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("###Value", &value.x, 0.001);
            break;
        case DataType::Float2:
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("###Value", &value.x, 0.001);
            break;
        case DataType::Float3:
            [[fallthrough]];
        case DataType::Float4:
            colorButton(value, backupValue, dataType);
            break;
        case DataType::Texture: {
            char const* textypenames[] = { "Texture2D", "Cubemap" };
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::Combo("###Texture Type", (int*)&textype, textypenames, 2);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (assetButton("###Texture", texture,
                            { .repr = AssetRepresentation::GPU,
                              .style = AssetButtonStyle::Thumbnail,
                              .size = ImGui::GetContentRegionAvail() }))
            {
                textureHandle = texture ? texture->handle() : AssetHandle{};
            }
            break;
        }
        }
    }

    void contextMenuContent() override {
        if (isParameter) {
            if (ImGui::MenuItem("Demote to Constant")) {
                node()->setName(nameAsConstant(dataType));
                isParameter = false;
            }
        }
        else {
            if (ImGui::MenuItem("Promote to Parameter")) {
                editor()->startRenamingNode(node());
                isParameter = true;
            }
        }
    }

    void didEditName(std::string newName) override { parameterName = newName; }

    float4 nodeColor() override {
        return isParameter ? colors.parameter : colors.constant;
    }
    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return false; }

    DataType dataType;
    float4 value = .5;
    float4 backupValue = {};
    AssetHandle textureHandle;
    Reference<Texture2D> texture;
    TextureSubType textype = TextureSubType::Texture2D;
    std::string parameterName;
    bool isParameter = false;
};

REGISTER_MATERIAL_NODE(ConstantController, {
    return enumMenu<DataType>("Constants",
                              defaultControllerFactory<DataType,
                                                       ConstantController>());
})

static DataTypeMask deduceBuiltinRetType(
    DataTypeMask definedRet, std::span<DataTypeMask const> definedArgs,
    std::span<DataTypeMask const> actualArgs) {
    BL_ASSERT(definedArgs.size() == actualArgs.size());
    using enum DataType;
    if (auto single = definedRet.getSingle()) {
        return *single;
    }
    int N = -1;
    for (auto [defined, actual]: zip(definedArgs, actualArgs)) {
        BL_ASSERT(FloatN.contains(defined));
        BL_ASSERT(FloatN.contains(actual));
        auto* ref = defined.count() == 1 ? &defined :
                    actual.count() == 1  ? &actual :
                                           nullptr;
        if (!ref) {
            continue;
        }
        if (auto single = ref->getSingle()) {
            int acceptLimit = defined.contains(Float) ? 1 : 2;
            int n = numComponents(*single);
            if (n >= acceptLimit) {
                N = n;
            }
        }
    }
    if (N != -1) {
        return FloatNType(N);
    }
    return definedRet;
}

static DataTypeMask getBuiltinRetType(BuiltinFunction function) {
    using enum DataType;
    switch (function) {
#define BL_MATGRAPH_BUILTIN(Name, Spelling, Ret, ...)                          \
    case BuiltinFunction::Name:                                                \
        return Ret;
#include "Bloom/MaterialCompiler/Lists.def"
    }
}

static utl::small_vector<DataTypeMask> getBuiltinArgTypes(
    BuiltinFunction function) {
    using enum DataType;
    switch (function) {
#define UNPACK(...) __VA_ARGS__
#define BL_MATGRAPH_BUILTIN(Name, Spelling, Ret, Args)                         \
    case BuiltinFunction::Name:                                                \
        return { UNPACK Args };
#include "Bloom/MaterialCompiler/Lists.def"
#undef UNPACK
    }
}

static auto getFunctionsImpl(BuiltinFunction first, BuiltinFunction last) {
    using enum BuiltinFunction;
    return iota(0, (int)last - (int)first + 1) |
           transform([=](int n) { return BuiltinFunction{ (int)first + n }; });
}

static auto getGeometricFunctions() {
    using enum BuiltinFunction;
    return getFunctionsImpl(Cross, Reflect);
}

static auto getMathFunctions() {
    using enum BuiltinFunction;
    return getFunctionsImpl(Acos, Trunc);
}

struct BuiltinFunctionController: OverloadableNodeController {
    static constexpr auto Name = "BuiltinFunction";

    static NodeDesc Make(BuiltinFunction function) {
        return { .name = std::string(magic_enum::enum_name(function)),
                 .controller =
                     std::make_unique<BuiltinFunctionController>(function) };
    }

    explicit BuiltinFunctionController(BuiltinFunction function):
        OverloadableNodeController(getBuiltinArgTypes(function) |
                                   ranges::to<std::vector>),
        function(function) {}

    explicit BuiltinFunctionController(YAML::Node const& root):
        OverloadableNodeController(root) {
        function = root["FunctionType"].as<BuiltinFunction>();
    }

    YAML::Node serialize() const override {
        YAML::Node root = OverloadableNodeController::serialize();
        root["Type"] = Name;
        root["FunctionType"] = function;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        auto argTypes = getBuiltinArgTypes(function);
        return argTypes | transform([](DataTypeMask m) {
            return PinDesc{ .userType = m };
        }) | ranges::to<std::vector>;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return {
            { .name = "Result",
              .userType = deduceBuiltinRetType(getBuiltinRetType(function),
                                               getBuiltinArgTypes(function),
                                               inputTypes()) },
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& g) const override {
        if (isAnyInputUntyped()) {
            return {};
        }
        auto inTypes = inputTypes() | transform([&](DataTypeMask m) {
            return translateType(ctx, m.getSingle().value());
        }) | ranges::to<std::vector>;
        auto* value = g.addValue<FunctionCall>(ctx, function, inTypes);
        return { value };
    }

    float4 nodeColor() override { return colors.util; }
    float2 preferredContentSize() const override { return { 70, 0 }; }
    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    BuiltinFunction function;
};

REGISTER_MATERIAL_NODE(BuiltinFunctionController, {
    if (auto n = enumMenu<BuiltinFunctionController>(getGeometricFunctions(),
                                                     "Geometric"))
        return n;
    if (auto n =
            enumMenu<BuiltinFunctionController>(getMathFunctions(), "Math"))
        return n;
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
                            .userType = mask(DataType::Float),
                            .flags = PinFlags::Optional };
        }) | ranges::to<std::vector>;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return {
            { .userType = mask(FloatNType(numComponents)) },
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        return { graph.addValue(
            allocate<VectorBuilder>(ctx.vectorType(numComponents),
                                    numComponents)) };
    }

    Value* defaultInputValue(Context& ctx, size_t) const override {
        return ctx.floatConstant(0);
    }
    float2 preferredContentSize() const override { return { 50, 0 }; }
    float4 nodeColor() override { return colors.util; }

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
            { .userType = mask(FloatNType(numComponents)) },
        };
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return std::vector<PinDesc>(numComponents,
                                    { .userType = mask(DataType::Float) });
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        return iota(0, numComponents) | transform([&](int i) {
            return graph.addValue(allocate<VectorElement>(ctx, i, nullptr));
        }) | ToSmallVector<>;
    }

    float4 nodeColor() override { return colors.util; }
    float2 preferredContentSize() const override { return { 50, 0 }; }
    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return false; }

    int numComponents;
};

REGISTER_MATERIAL_NODE(SplitController, {
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
        OverloadableNodeController(2, DataTypeMask::FloatN), operation(op) {}

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
        return { { .name = "A", .userType = DataTypeMask::FloatN },
                 { .name = "B", .userType = DataTypeMask::FloatN } };
    }

    DataTypeMask selectOutputType() const {
        using enum DataType;
        auto a = inputType(0);
        auto b = inputType(1);
        if (a == DataTypeMask(Float)) {
            return b;
        }
        if (b == DataTypeMask(Float)) {
            return a;
        }
        return a.count() < b.count() ? b : a;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return { { .name = "Result", .userType = selectOutputType() } };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        if (isAnyInputUntyped()) {
            return {};
        }
        auto* value = graph.addValue<ArithmeticOperation>(
            operation, translateType(ctx, inputType(0).getSingle().value()),
            translateType(ctx, inputType(1).getSingle().value()));
        return { value };
    }

    float4 nodeColor() override { return colors.util; }
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
        using enum DataType;
        return { { .name = "Color", .userType = mask(Float3) },
                 { .name = ColorChannelNames[0],
                   .flags = PinFlags::Optional,
                   .userType = mask(Float),
                   .color = ColorChannelColors[0] },
                 { .name = ColorChannelNames[1],
                   .flags = PinFlags::Optional,
                   .userType = mask(Float),
                   .color = ColorChannelColors[1] },
                 { .name = ColorChannelNames[2],
                   .flags = PinFlags::Optional,
                   .userType = mask(Float),
                   .color = ColorChannelColors[2] },
                 { .name = ColorChannelNames[3],
                   .flags = PinFlags::Optional,
                   .userType = mask(Float) } };
    }

    static DataType getUVType(TextureSubType subtype) {
        using enum DataType;
        switch (subtype) {
        case TextureSubType::Texture2D:
            return Float2;
        case TextureSubType::TextureCube:
            return Float3;
        }
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return {
            { .name = "Texture", .userType = mask(DataType::Texture) },
            { .name = "UVs",
              .flags = PinFlags::Optional,
              .userType = mask(getUVType(subtype)) },
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

    float4 nodeColor() override { return colors.sampler; }
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
