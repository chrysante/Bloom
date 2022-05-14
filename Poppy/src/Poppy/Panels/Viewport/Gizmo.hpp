#pragma once

#include <string>

#include "Bloom/Graphics/Camera.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Application/Input.hpp"
#include "Poppy/SelectionContext.hpp"

namespace poppy {
	
    class Gizmo {
	public:
		enum class Operation {
			translate, rotate, scale
		};

		enum class Space {
			world, local
		};
		
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
		
		void display(bloom::Camera const&, bloom::Scene*, SelectionContext*);
		
	private:
		struct ImGuizmoCtx;
		struct ImGuizmoDeleter {
			void operator()(ImGuizmoCtx*) const;
		};
		
    private:
		std::unique_ptr<ImGuizmoCtx, ImGuizmoDeleter> context;
		bloom::Input const* _input = nullptr;
		
		Operation _operation = Operation::translate;
		Space _space[3] = { Space::world, Space::world, Space::local };
		bool _hovered = false;
		bool _using = false;
    };

	std::string toString(Gizmo::Operation);
	std::string toString(Gizmo::Space);
	
}
