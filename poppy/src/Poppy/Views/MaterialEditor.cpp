#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;

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

} // namespace

POPPY_REGISTER_VIEW(MaterialEditor, "Material Editor");

MaterialEditor::MaterialEditor(): nodeEditor(commandPaletteWrite()) {
    auto& cmd = commandPaletteWrite();
    auto* compileCmd = cmd.add("Compile", { Key::B, ModFlags::Control },
                               [this] { compile(); });
    setToolbar({ ToolbarIconButton(compileCmd, "ok") });
    setPadding({ 0, 0 });
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
