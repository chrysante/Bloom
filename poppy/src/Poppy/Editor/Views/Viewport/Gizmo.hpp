#pragma once

#include <string>

#include "Bloom/Application/Input.hpp"
#include "Bloom/Graphics/Camera.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "Poppy/Editor/SelectionContext.hpp"

namespace poppy {

class Gizmo {
public:
    enum class Operation { translate, rotate, scale, _count };

    enum class Space { world, local, _count };

public:
    Gizmo();

    Operation operation() const { return _operation; }
    void setOperation(Operation);

    void setInput(bloom::Input const& input) { _input = &input; }

    Space space() const { return _space[(std::size_t)_operation]; }
    void setSpace(Space);
    void cycleSpace();

    bool isHovered() const { return _hovered; }
    bool isUsing() const { return _using; }

    void display(bloom::Camera const&, SelectionContext&);

private:
    struct ImGuizmoCtx;
    struct ImGuizmoDeleter {
        void operator()(ImGuizmoCtx*) const;
    };

private:
    std::unique_ptr<ImGuizmoCtx, ImGuizmoDeleter> context;
    bloom::Input const* _input = nullptr;

    Operation _operation = Operation::translate;
    Space _space[3]      = { Space::world, Space::world, Space::local };
    bool _hovered        = false;
    bool _using          = false;
};

std::string toString(Gizmo::Operation);
std::string toString(Gizmo::Space);

} // namespace poppy
