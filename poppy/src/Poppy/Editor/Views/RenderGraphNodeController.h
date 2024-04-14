#ifndef POPPY_EDITOR_VIEWS_RENDERGRAPHNODECONTROLLER_H
#define POPPY_EDITOR_VIEWS_RENDERGRAPHNODECONTROLLER_H

#include <functional>
#include <string>
#include <vector>

#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Poppy/Editor/Views/NodeEditor.h"

namespace poppy {

class RenderGraphNodeController {
public:
    virtual ~RenderGraphNodeController() = default;

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
};

///
class PassRegistry {
public:
    struct Entry {
        std::string name;
        std::function<nodeEditor::NodeDesc()> create;
    };

    auto begin() const { return entries.begin(); }
    auto end() const { return entries.end(); }

    static PassRegistry const& get();

private:
    friend void registerPass(Entry entry);

    std::vector<Entry> entries;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_RENDERGRAPHNODECONTROLLER_H
