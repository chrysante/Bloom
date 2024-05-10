#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <magic_enum.hpp>
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

enum class DataType { Float, Float2, Float3, Float4, Texture };

enum class NodeType {
    Output,
    Input,
    Constant,
    Append,
    Split,
    Arithmetic,
    TextureSampler
};

enum class InputKind { TextureCoords };

enum class ArithmeticOp { Add, Sub, Mul, Div, LAST = Div };

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
                    int numComponents =
                        (int)input->userType() - (int)DataType::Float + 1;
                    return ctx.vectorConstant(numComponents, {});
                }();
                user->setOperand(inputIndex, operand);
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
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        auto* type = ctx.aggregateType(std::array<Type const*, 5>{
            ctx.float3Type(),
            ctx.float3Type(),
            ctx.floatType(),
            ctx.floatType(),
            ctx.floatType(),
        });
        return { graph.setRoot(allocate<AggregateBuilder>(type)) };
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
        case Float:
            desc.minSize = { 100, 50 };
            break;
        case Float2:
            desc.minSize = { 100, 50 };
            break;
        case Float3:
            desc.minSize = { 200, 200 };
            desc.resizable = true;
            break;
        case Float4:
            desc.minSize = { 200, 200 };
            desc.resizable = true;
            break;
        case Texture:
            desc.minSize = { 200, 200 };
            desc.resizable = true;
            break;
        }
        return desc;
    }

    explicit ConstantController(DataType dataType): dataType(dataType) {}

    explicit ConstantController(YAML::Node const& node) {
        dataType = node["DataType"].as<DataType>();
        value = node["Value"].as<float4>();
        textureAsset = node["Texture"].as<AssetHandle>();
    }

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = NodeType::Constant;
        node["DataType"] = dataType;
        node["Value"] = value;
        node["Texture"] = textureAsset;
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
        case Texture:
            return { ctx.textureConstant(textureAsset) };
        }
    }

    bool alwaysDisplayInputNames() const override { return false; }

    bool alwaysDisplayOutputNames() const override { return false; }

    float4 nodeColor() override { return { 0.2, .8, .2, 1 }; }

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
        case DataType::Texture:
            if (auto asset = acceptAssetDragDrop({ AssetType::Texture2D })) {
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
            if (texHandle && texHandle.nativeHandle()) {
                ImGui::Image(texHandle.nativeHandle(),
                             ImGui::GetContentRegionAvail());
            }
            break;
        }
    }

    DataType dataType;
    float4 value = .5;
    AssetHandle textureAsset;
    TextureHandle texHandle;
    bool triedToGetTexture = false;
};

/// Any fragment shader input, i.e., rasterizer data
struct InputNodeController: MaterialNodeController {
    static NodeDesc Make(InputKind kind, float2 position) {
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

    std::vector<PinDesc> makeInputDescriptors() override {
        switch (kind) {
        case InputKind::TextureCoords:
            return {};
        }
    }

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
        }
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& g) const override {
        switch (kind) {
        case InputKind::TextureCoords: {
            auto* coords = ctx.textureCoords();
            return { coords,
                     g.addValue(allocate<VectorElement>(ctx, 0, coords)),
                     g.addValue(allocate<VectorElement>(ctx, 1, coords)) };
        }
        }
    }

    bool alwaysDisplayInputNames() const override { return false; }
    bool alwaysDisplayOutputNames() const override { return true; }

    InputKind kind;
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

struct ArithmeticNodeController: MaterialNodeController {
    static NodeDesc Make(ArithmeticOp op, float2 position) {
        return { .name = std::string(magic_enum::enum_name(op)),
                 .position = position,
                 .controller = std::make_unique<ArithmeticNodeController>(op) };
    }

    ArithmeticNodeController(ArithmeticOp op): operation(op) {}

    explicit ArithmeticNodeController(YAML::Node const& root) {
        operation = root["Operation"].as<ArithmeticOp>();
    }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Type"] = NodeType::Arithmetic;
        root["Operation"] = operation;
        return root;
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return { { .name = "A",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float },
                 { .name = "B",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float } };
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return { { .name = "Result",
                   .userType = (PinUserType)DataType::Float } };
    }

    utl::small_vector<Value*> createGraphNodes(Context&,
                                               MaterialGraph&) const override {
        BL_UNIMPLEMENTED();
    }

    bool alwaysDisplayInputNames() const override { return false; }

    bool alwaysDisplayOutputNames() const override { return false; }

    ArithmeticOp operation;
};

struct TextureSamplerController: MaterialNodeController {
    static NodeDesc Make(float2 position) {
        return { .name = "Texture Sample",
                 .position = position,
                 .controller = std::make_unique<TextureSamplerController>() };
    }

    TextureSamplerController() = default;

    explicit TextureSamplerController(YAML::Node const&) {}

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = NodeType::TextureSampler;
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

    std::vector<PinDesc> makeInputDescriptors() override {
        return {
            { .name = "Texture",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Texture },
            { .name = "UVs",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float2 },
        };
    }

    utl::small_vector<Value*> createGraphNodes(
        Context& ctx, MaterialGraph& graph) const override {
        auto* sample =
            graph.addValue(allocate<TextureSampler>(ctx, nullptr, nullptr));
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
    case NodeType::Arithmetic:
        return std::make_unique<ArithmeticNodeController>(node);
    case NodeType::TextureSampler:
        return std::make_unique<TextureSamplerController>(node);
    }
}

struct MaterialModelImpl: bloom::MaterialModel {
    bool hasCPURepr() const override { return !model.empty(); }

    YAML::Node serialize() const override { return model.serialize(); }

    void deserialize(YAML::Node const& node) override {
        model.deserialize(node, [](YAML::Node const& node) {
            return deserializeNodeController(node["Type"].as<NodeType>(), node);
        });
    }

    static MaterialNodeController* getController(Node* node) {
        return dynamic_cast<MaterialNodeController*>(node->controller());
    }

    MaterialSourceData generateSource() const override {
        Context ctx;
        MaterialGraph graph;
        MaterialNodeController::ValueMap map;
        for (auto* node: model.nodes()) {
            auto* controller = getController(node);
            map[node] = controller->createGraphNodes(ctx, graph);
        }
        for (auto* node: model.nodes()) {
            getController(node)->linkGraphNode(ctx, map);
        }
        auto codegen = makeCodeGenerator(ctx, graph, {});
        auto result = codegen->run();
        return {
            .source = std::move(result.source),
            .fragmentTextures = std::move(result.fragmentTextures),
        };
    }

    mutable NodeEditorGraph model;
};

} // namespace

static NodeEditorGraph* getModel(Material* material) {
    if (!material) {
        return nullptr;
    }
    return &dynamic_cast<MaterialModelImpl*>(material->model.get())->model;
}

std::unique_ptr<bloom::MaterialModel> bloom::makeMaterialModel() {
    return std::make_unique<MaterialModelImpl>();
}

namespace {

struct MaterialEditor: View {
    MaterialEditor();

    void display() override;

    void onInput(InputEvent& event) override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    void doOpenData(std::any data) override {
        if (auto* mat = std::any_cast<Reference<Material>>(&data)) {
            material = *mat;
            nodeEditor.setModel(getModel(material.get()));
        }
    }

    void compile();

    Reference<Material> material;
    NodeEditor nodeEditor;
};

struct Delegate: NodeEditorDelegate {
    Delegate(MaterialEditor& editor): editor(editor) {}

    void backgroundPopupMenu(vml::float2 position) override {
        auto* model = getModel();
        if (!model) {
            return;
        }
        if (ImGui::MenuItem("Output")) {
            model->addNode(OutputNodeController::Make(position));
        }
        if (ImGui::MenuItem("Texture Coords")) {
            model->addNode(
                InputNodeController::Make(InputKind::TextureCoords, position));
        }
        if (ImGui::BeginMenu("Append")) {
            for (int i = 2; i <= 4; ++i) {
                if (ImGui::MenuItem(utl::strcat("Float", i).c_str())) {
                    model->addNode(AppendNodeController::Make(position, i));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Split")) {
            for (int i = 2; i <= 4; ++i) {
                if (ImGui::MenuItem(utl::strcat("Float", i).c_str())) {
                    model->addNode(SplitNodeController::Make(position, i));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Constant")) {
            for (auto type: magic_enum::enum_values<DataType>()) {
                if (ImGui::MenuItem(
                        std::string(magic_enum::enum_name(type)).c_str()))
                {
                    model->addNode(ConstantController::Make(position, type));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Texture Sample")) {
            model->addNode(TextureSamplerController::Make(position));
        }
        for (size_t i = 0; i < EnumCount<ArithmeticOp>; ++i) {
            auto op = (ArithmeticOp)i;
            if (ImGui::MenuItem(std::string(magic_enum::enum_name(op)).c_str()))
            {
                model->addNode(ArithmeticNodeController::Make(op, position));
            }
        }
    }

    NodeEditorGraph* getModel() { return ::getModel(editor.material.get()); }

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
        ToolbarIconButton("plus")
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
    nodeEditor.display();
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
