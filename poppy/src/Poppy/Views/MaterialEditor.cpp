#include <vector>

#include <magic_enum.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Yaml.h"
#include "Bloom/Graphics/Material/Context.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialGraph.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;
using namespace nodeEditor;
using namespace matgraph;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

enum class DataType { Float, Float2, Float3, Float4, Texture };

enum class NodeType { Output, Constant, Arithmetic, TextureSampler };

enum class ArithmeticOp { Add, Sub, Mul, Div, LAST = Div };

struct MaterialNodeController: NodeController {
    struct ValueMap: utl::hashmap<Node*, Value*> {
        Value* get(Node* node) const {
            auto itr = find(node);
            BL_ASSERT(itr != end());
            return itr->second;
        }
        template <typename T>
        T* get(Node* node) const {
            return dyncast<T*>(get(node));
        }
    };

    virtual Value* createGraphNode(MaterialGraph& graph) const = 0;

    virtual void linkGraphNode(ValueMap const& map) const {
        auto* user = map.get<User>(node());
        if (!user) {
            return;
        }
        for (auto [index, input]: node()->inputs() | enumerate) {
            if (input->origin()) {
                // FIXME: This only works for nodes with single outputs
                auto* operand = map.get(&input->origin()->parent());
                user->setOperand(index, operand);
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
            { .name = "Roughness",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float },
            { .name = "Metallic",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float },
            { .name = "Emissive",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float3 },
        };
    }

    Value* createGraphNode(MaterialGraph& graph) const override {
        auto& ctx = graph.context();
        auto* type = ctx.aggregateType(std::array<Type const*, 4>{
            ctx.float3Type(),
            ctx.floatType(),
            ctx.floatType(),
            ctx.float3Type(),
        });
        return graph.setRoot(allocate<AggregateBuilder>(type));
    }

    vml::float4 nodeColor() override { return { 0.1, 0.1, 0.1, 1 }; }
};

struct ConstantController: MaterialNodeController {
    static NodeDesc Make(float2 position, DataType dataType) {
        return { .name = "Constant",
                 .position = position,
                 .controller = std::make_unique<ConstantController>(dataType) };
    }

    explicit ConstantController(DataType dataType): dataType(dataType) {}

    explicit ConstantController(YAML::Node const& node) {
        dataType = node["DataType"].as<DataType>();
        value = node["Value"].as<float4>();
    }

    YAML::Node serialize() const override {
        YAML::Node node;
        node["Type"] = NodeType::Constant;
        node["DataType"] = dataType;
        node["Value"] = value;
        return node;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return {
            { .name = "Value", .userType = (PinUserType)dataType },
        };
    }

    Value* createGraphNode(MaterialGraph& graph) const override {
        BL_ASSERT(dataType == DataType::Float, "Other cases not implemented");
        return graph.addValue(
            allocate<FloatConstant>(graph.context(), value.x));
    }

    bool alwaysDisplayInputNames() const override { return false; }

    bool alwaysDisplayOutputNames() const override { return true; }

    float4 nodeColor() override { return { 0.2, .8, .2, 1 }; }

    DataType dataType;
    float4 value = 0;
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

    Value* createGraphNode(MaterialGraph&) const override {
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
                   .color = float4{ 1, 0, 0, 1 } },
                 { .name = "Green",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = float4{ 0, 1, 0, 1 } },
                 { .name = "Blue",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float,
                   .color = float4{ 0, 0.5, 1, 1 } },
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

    Value* createGraphNode(MaterialGraph&) const override {
        BL_UNIMPLEMENTED();
    }

    vml::float4 nodeColor() override { return { 0.2, 0.4, 1, 1 }; }
};

static std::unique_ptr<NodeController> deserializeNodeController(
    NodeType type, YAML::Node const& node) {
    switch (type) {
    case NodeType::Output:
        return std::make_unique<OutputNodeController>(node);
    case NodeType::Constant:
        return std::make_unique<ConstantController>(node);
    case NodeType::Arithmetic:
        return std::make_unique<ArithmeticNodeController>(node);
    case NodeType::TextureSampler:
        return std::make_unique<TextureSamplerController>(node);
    }
}

struct MaterialModelImpl: bloom::MaterialModel {
    YAML::Node serialize() const { return model.serialize(); }

    void deserialize(YAML::Node const& node) {
        model.deserialize(node, [](YAML::Node const& node) {
            return deserializeNodeController(node["Type"].as<NodeType>(), node);
        });
    }

    void compile(Material&) const {}

    NodeEditorGraph model;
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
        if (ImGui::MenuItem("Constant")) {
            model->addNode(ConstantController::Make(position, DataType::Float));
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

static MaterialNodeController* getController(Node* node) {
    return dynamic_cast<MaterialNodeController*>(node->controller());
}

void MaterialEditor::compile() {
    auto* model = getModel(material.get());
    BL_ASSERT(model);
    MaterialGraph graph;
    MaterialNodeController::ValueMap map;
    for (auto* node: model->nodes()) {
        auto* controller = getController(node);
        auto* value = controller->createGraphNode(graph);
        map[node] = value;
    }
    for (auto* node: model->nodes()) {
        getController(node)->linkGraphNode(map);
    }
}
