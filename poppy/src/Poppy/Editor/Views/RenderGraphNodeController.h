#ifndef POPPY_EDITOR_VIEWS_RENDERGRAPHNODECONTROLLER_H
#define POPPY_EDITOR_VIEWS_RENDERGRAPHNODECONTROLLER_H

#include <functional>
#include <string>
#include <vector>

#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Poppy/Editor/Views/NodeEditor.h"

namespace poppy {

class RenderGraphNodeController: public nodeEditor::NodeController {
public:
    /// Creates this node in \p renderGraph
    virtual void create(bloom::rg::RenderGraph& renderGraph) = 0;

    /// \Returns the source at index \p index
    /// \Param index is the index in the node editor UI
    /// \Pre \p index must be valid and `create(RenderGraph)` must have been
    /// called before
    virtual bloom::rg::PassSource* getSource(size_t index) const = 0;

    /// \Returns the sink at index \p index
    /// \Param index is the index in the node editor UI
    /// \Pre \p index must be valid and `create(RenderGraph)` must have been
    /// called before
    virtual bloom::rg::PassSink* getSink(size_t index) const = 0;

    /// Counterpart to the non-static `NodeController::serialize`
    static std::unique_ptr<RenderGraphNodeController> deserialize(
        YAML::Node node);
};

///
class PassRegistry {
public:
    struct Entry {
        std::string name;
        std::function<nodeEditor::NodeDesc()> factory;
    };

    auto begin() const { return entries.begin(); }
    auto end() const { return entries.end(); }

    static PassRegistry const& get();

private:
    friend Entry& registerPass();

    std::vector<Entry> entries;
};

/// Constructs a render graph from the node editor model \p model
std::unique_ptr<bloom::rg::RenderGraph> constructRenderGraph(
    bloom::rg::RenderGraphContext& ctx, bloom::HardwareDevice& device,
    NodeEditorGraph const& model);

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_RENDERGRAPHNODECONTROLLER_H
