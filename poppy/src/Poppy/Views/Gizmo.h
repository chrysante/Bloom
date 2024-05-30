#ifndef POPPY_VIEWS_GIZMO_H
#define POPPY_VIEWS_GIZMO_H

#include <string>

#include "Bloom/Core/EnumCount.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/EntitySelectionManager.h"

namespace poppy {

/// List of operations a gizmo can perform
enum class GizmoOperation { Translate, Rotate, Scale, LAST = Scale };

/// Converts \p operation to a string
std::string toString(GizmoOperation operation);

/// List of space relations of a gizmo
enum class GizmoSpace { World, Local, LAST = Local };

/// Converts \p space to a string
std::string toString(GizmoSpace space);

/// 3D gizmo
class Gizmo {
public:
    Gizmo();

    /// \Returns the operation state of the gizmo
    GizmoOperation operation() const { return _operation; }

    /// Sets the operation state of the gizmo to \p operation
    void setOperation(GizmoOperation operation);

    /// \Returns the space state of the gizmo
    GizmoSpace space() const { return _space[_operation]; }

    /// Sets the space state of the gizmo to \p space
    void setSpace(GizmoSpace space);

    /// Cycles the space state
    void cycleSpace();

    /// \Returns `true` if the gizmo is hovered
    bool isHovered() const { return _hovered; }

    /// \Returns `true` if the gizmo is in use, i.e., edits have been made and
    /// the operation is not finished
    bool isUsing() const { return _using; }

    /// Displays the gizmo
    /// \Returns `true` if any edits where made this frame
    bool display(bloom::Camera const& camera, vml::float4x4& wsTransform);

    /// Resets the gizmo to unused state
    void reset() {
        _hovered = false;
        _using = false;
    }

private:
    struct ContextDeleter {
        void operator()(void*) const;
    };

    std::unique_ptr<void, ContextDeleter> context;
    GizmoOperation _operation = GizmoOperation::Translate;
    /// We store the space for each operation separately
    bloom::EnumArray<GizmoOperation, GizmoSpace> _space = { GizmoSpace::World,
                                                            GizmoSpace::World,
                                                            GizmoSpace::Local };
    bool _hovered = false;
    bool _using = false;
};

} // namespace poppy

#endif // POPPY_VIEWS_GIZMO_H
