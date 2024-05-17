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
#include "Poppy/Views/MaterialNodeController.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace bloom;
using namespace nodeEditor;
using namespace matc;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

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
            return deserializeMaterialNodeController(node);
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
            if (!controller) {
                throw std::runtime_error(
                    utl::strcat("Node has no controller: ", node->name()));
            }
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

    Node* createNodePopupMenu() override {
        auto* editorGraph = &getModel()->editorGraph;
        if (!editorGraph) {
            return nullptr;
        }
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

    bool isPinTypeCompatible(PinUserType output,
                             PinUserType input) const override {
        return isDataTypeCompatible((DataType)output, (DataType)input);
    }

    std::unique_ptr<nodeEditor::NodeController> deserializeNodeController(
        YAML::Node const& node) override {
        return deserializeMaterialNodeController(node);
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
