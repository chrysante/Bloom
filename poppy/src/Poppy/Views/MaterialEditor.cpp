#include <vector>

#include <vml/vml.hpp>

#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;
using namespace nodeEditor;
using namespace vml::short_types;

namespace {

struct MaterialEditor: View {
    MaterialEditor();

    void display() override;

    void onInput(InputEvent& event) override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    void compile();

    NodeEditor nodeEditor;
};

enum class DataType { Float, Float2, Float3, Float4, Texture };

struct OutputNodeController: NodeController {
    static NodeDesc Make(float2 position) {
        return { .name = "Output",
                 .position = position,
                 .controller = std::make_unique<OutputNodeController>() };
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

enum class ArithmeticOperation { Add, Sub, Mul, Div, LAST = Div };

static std::string toString(ArithmeticOperation op) {
    return std::array{ "Add", "Sub", "Mul", "Div" }[(size_t)op];
}

struct ArithmeticNodeController: NodeController {
    static NodeDesc Make(ArithmeticOperation op, float2 position) {
        return { .name = toString(op),
                 .position = position,
                 .controller = std::make_unique<ArithmeticNodeController>(op) };
    }

    ArithmeticNodeController(ArithmeticOperation op): operation(op) {}

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

    std::vector<PinDesc> makeOutputDescriptors() override {
        return { { .name = "Color", .userType = (PinUserType)DataType::Float3 },
                 { .name = "Red",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float },
                 { .name = "Green",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float },
                 { .name = "Blue",
                   .flags = PinFlags::Optional,
                   .userType = (PinUserType)DataType::Float },
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

struct Delegate: NodeEditorDelegate {
    Delegate(MaterialEditor& editor): editor(editor) {}

    void backgroundPopupMenu(vml::float2 position) override {
        if (ImGui::MenuItem("Output")) {
            editor.nodeEditor.addNode(OutputNodeController::Make(position));
        }
        if (ImGui::MenuItem("Texture Sample")) {
            editor.nodeEditor.addNode(TextureSamplerController::Make(position));
        }
        for (size_t i = 0; i < EnumCount<ArithmeticOperation>; ++i) {
            auto op = (ArithmeticOperation)i;
            if (ImGui::MenuItem(toString(op).c_str())) {
                editor.nodeEditor.addNode(
                    ArithmeticNodeController::Make(op, position));
            }
        }
    }

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
        ToolbarTextItem("Material Name Here")
            .font(FontDesc::UIDefault().setWeight(FontWeight::Bold)),
        ToolbarSpacer(),
        ToolbarIconButton("plus")
    }); // clang-format on
    setPadding({ 0, 0 });
    nodeEditor.setDelegate(std::make_unique<Delegate>(*this));
}

void MaterialEditor::display() { nodeEditor.display(); }

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
