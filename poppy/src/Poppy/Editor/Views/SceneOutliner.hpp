#pragma once

#include <optional>
#include <span>

#include <utl/hashset.hpp>
#include <utl/vector.hpp>

#include "Bloom/Scene/Entity.hpp"
#include "Poppy/Editor/Views/BasicSceneInspector.hpp"
#include "Poppy/UI/View.hpp"
#include "PropertiesView.hpp"

namespace bloom {

class Scene;

}

namespace poppy {

class SceneOutliner: public View, public BasicSceneInspector {
public:
    SceneOutliner();

private:
    void frame() override;
    void onInput(bloom::InputEvent&) override;

    struct TreeNodeDescription {
        std::size_t id = 0;
        bool selected = false;
        bool expanded = false;
        bool isLeaf = true;
        std::string_view name;
    };
    void treeNode(TreeNodeDescription const&, auto&& block);
    void displayEntity(bloom::EntityHandle);
    void displayScene(bloom::Scene&);

    void dragDropSource(bloom::EntityHandle child);
    void dragDropTarget(bloom::EntityHandle parent);

    bool expanded(bloom::Scene const*) const;
    bool expanded(bloom::ConstEntityHandle) const;
    void setExpanded(bloom::Scene const*, bool);
    void setExpanded(bloom::ConstEntityHandle, bool);

private:
    utl::hashset<bloom::ConstEntityHandle> mExpanded;
};

} // namespace poppy
