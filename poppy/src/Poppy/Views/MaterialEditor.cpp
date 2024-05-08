#include <vector>

#include <vml/vml.hpp>

#include "Bloom/Graphics/Material/Material.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;
using namespace nodeEditor;
using namespace vml::short_types;

namespace {

enum class DataType { Float, Float2, Float3, Float4, Texture };

enum class NodeType { Output, Arithmetic, TextureSampler };

enum class ArithmeticOperation { Add, Sub, Mul, Div, LAST = Div };

} // namespace

template <>
struct YAML::convert<DataType> {
    static Node encode(DataType type) {
        Node node;
        node = std::array{ "Float", "Float2", "Float3", "Float4",
                           "Texture" }[(size_t)type];
        return node;
    }

    static bool decode(DataType& type, Node const& node) {
        auto text = node.as<std::string>();
        if (text == "Float") {
            type = DataType::Float;
            return true;
        }
        if (text == "Float2") {
            type = DataType::Float2;
            return true;
        }
        if (text == "Float3") {
            type = DataType::Float3;
            return true;
        }
        if (text == "Float4") {
            type = DataType::Float4;
            return true;
        }
        if (text == "Texture") {
            type = DataType::Texture;
            return true;
        }
        return false;
    }
};

template <>
struct YAML::convert<::NodeType> {
    static Node encode(::NodeType type) {
        Node node;
        node = std::array{ "Output", "Arithmetic",
                           "TextureSampler" }[(size_t)type];
        return node;
    }

    static bool decode(Node const& node, ::NodeType& type) {
        auto text = node.as<std::string>();
        if (text == "Output") {
            type = ::NodeType::Output;
            return true;
        }
        if (text == "Arithmetic") {
            type = ::NodeType::Arithmetic;
            return true;
        }
        if (text == "TextureSampler") {
            type = ::NodeType::TextureSampler;
            return true;
        }
        if (text == "Output") {
            type = ::NodeType::Output;
            return true;
        }
        if (text == "Arithmetic") {
            type = ::NodeType::Arithmetic;
            return true;
        }
        return false;
    }
};

static std::string toString(ArithmeticOperation op) {
    return std::array{ "Add", "Sub", "Mul", "Div" }[(size_t)op];
}

template <>
struct YAML::convert<ArithmeticOperation> {
    static Node encode(ArithmeticOperation op) {
        Node node;
        node = toString(op);
        return node;
    }

    static bool decode(Node const& node, ArithmeticOperation& op) {
        auto text = node.as<std::string>();
        if (text == "Add") {
            op = ArithmeticOperation::Add;
            return true;
        }
        if (text == "Sub") {
            op = ArithmeticOperation::Sub;
            return true;
        }
        if (text == "Mul") {
            op = ArithmeticOperation::Mul;
            return true;
        }
        if (text == "Div") {
            op = ArithmeticOperation::Div;
            return true;
        }
        return false;
    }
};

namespace {

struct OutputNodeController: NodeController {
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
            { .name = "Ambient Occlusion",
              .flags = PinFlags::Optional,
              .userType = (PinUserType)DataType::Float },
        };
    }

    vml::float4 nodeColor() override { return { 0.1, 0.1, 0.1, 1 }; }
};

struct ArithmeticNodeController: NodeController {
    static NodeDesc Make(ArithmeticOperation op, float2 position) {
        return { .name = toString(op),
                 .position = position,
                 .controller = std::make_unique<ArithmeticNodeController>(op) };
    }

    ArithmeticNodeController(ArithmeticOperation op): operation(op) {}

    explicit ArithmeticNodeController(YAML::Node const& root) {
        operation = root["Operation"].as<ArithmeticOperation>();
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

    bool alwaysDisplayInputNames() const override { return false; }

    bool alwaysDisplayOutputNames() const override { return false; }

    ArithmeticOperation operation;
};

struct TextureSamplerController: NodeController {
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

    vml::float4 nodeColor() override { return { 0.2, 0.4, 1, 1 }; }
};

static std::unique_ptr<NodeController> deserializeNodeController(
    NodeType type, YAML::Node const& node) {
    switch (type) {
    case NodeType::Output:
        return std::make_unique<OutputNodeController>(node);
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
        if (ImGui::MenuItem("Texture Sample")) {
            model->addNode(TextureSamplerController::Make(position));
        }
        for (size_t i = 0; i < EnumCount<ArithmeticOperation>; ++i) {
            auto op = (ArithmeticOperation)i;
            if (ImGui::MenuItem(toString(op).c_str())) {
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
                               [this] { compile(); });
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

void MaterialEditor::compile() {}
