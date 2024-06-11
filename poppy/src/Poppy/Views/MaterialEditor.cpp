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
#include "Bloom/MaterialCompiler/Codegen.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/MaterialNodeController.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;
using namespace nodeEditor;
using namespace matc;
using namespace vml::short_types;
using namespace ranges::views;

namespace poppy {

void displayMaterialNodeColorEditor();

}

BLOOM_MAKE_TEXT_SERIALIZER(matc::CodegenParameters, shadingModel, transparency,
                           twoSided);

namespace {

struct MaterialModelImpl: bloom::MaterialModel {
    bool hasCPURepr() const override { return !editorGraph.empty(); }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["CodegenParameters"] = params;
        root["NodeGraph"] = editorGraph.serialize();
        return root;
    }

    void deserialize(YAML::Node const& node) override {
        editorGraph.deserialize(node["NodeGraph"], [](YAML::Node const& node) {
            return deserializeMaterialNodeController(node);
        });
        auto codegenParams = node["CodegenParameters"].as<CodegenParameters>(
            CodegenParameters{});
        setParameters(codegenParams);
    }

    static MaterialNodeController* getController(Node* node) {
        return dynamic_cast<MaterialNodeController*>(node->controller());
    }

    MaterialSourceData generateSource() const override {
        Context ctx;
        MaterialGraph graph;
        MaterialNodeController::ValueMap map;
        for (auto* node: editorGraph.nodes()) {
            auto* controller = getController(node);
            if (!controller) {
                throw std::runtime_error(
                    utl::strcat("Node has no controller: ", node->name()));
            }
            map[node] = controller->createGraphNodes(ctx, graph);
        }
        for (auto* node: editorGraph.nodes()) {
            getController(node)->linkGraphNode(ctx, map);
        }
        return matc::generateSource(ctx, graph, params);
    }

    CodegenParameters const& parameters() const { return params; }

    void setParameters(CodegenParameters p) {
        params = p;
        for (auto* node: editorGraph.nodes()) {
            auto* controller =
                dynamic_cast<MaterialNodeController*>(node->controller());
            if (controller) {
                controller->updateCodegenParameters(params);
            }
        }
    }

    NodeEditorGraph* getEditorGraph() const { return &editorGraph; }

private:
    mutable NodeEditorGraph editorGraph;
    CodegenParameters params;
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
    void receiveData(std::any data) override {
        if (auto* mat = std::any_cast<Reference<Material>>(&data)) {
            material = *mat;
            auto* model = getModel(material.get());
            nodeEditor.setModel(model->getEditorGraph());
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

    Node* createNodePopupMenu() override {
        auto* editorGraph = getModel()->getEditorGraph();
        std::unique_ptr<Node> node;
        for (auto& entry: getMaterialNodeRegistry()) {
            if (auto n = entry.menu()) {
                node = std::move(n);
            }
        }
        if (node) {
            return editorGraph->addNode(std::move(node));
        }
        return nullptr;
    }

    void backgroundContextMenu() override {
        ImGui::MenuItem("Display Color Editor", "", &displayColorEditor);
    }

    bool isPinTypeCompatible(PinUserType output,
                             PinUserType input) const override {
        return isDataTypeCompatible(output, input);
    }

    std::unique_ptr<nodeEditor::NodeController> deserializeNodeController(
        YAML::Node const& node) override {
        return deserializeMaterialNodeController(node);
    }

    MaterialModelImpl* getModel() { return ::getModel(editor.material.get()); }

    MaterialEditor& editor;
    bool displayColorEditor = false;
};

} // namespace

POPPY_REGISTER_VIEW(MaterialEditor, "Material Editor");

MaterialEditor::MaterialEditor():
    View({ .defaultSize = { 1000, 800 } }), nodeEditor(commandPaletteWrite()) {
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
                del->createNodePopupMenu();
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
    if (auto* del = dynamic_cast<Delegate*>(nodeEditor.getDelegate());
        del && del->displayColorEditor)
    {
        displayMaterialNodeColorEditor();
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
    fullWidth();
    auto params = model->parameters();
    bool modified = false;
    if (auto value = enumCombo2("###Shading Model", params.shadingModel)) {
        params.shadingModel = *value;
        modified = true;
    }
    beginProperty("Transparency");
    fullWidth();
    if (auto value = enumCombo2("###Transparency", params.transparency)) {
        params.transparency = *value;
        modified = true;
    }
    beginProperty("Two Sided");
    fullWidth();
    modified |= ImGui::Checkbox("###Two Sided", &params.twoSided);
    endSection();
    if (modified) {
        model->setParameters(params);
    }
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
        auto& AM = editor().assetManager();
        AM.makeAvailable(material->handle(), AssetRepresentation::GPU,
                         /* forceReload: */ true);
    });
}
