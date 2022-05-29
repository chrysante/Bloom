#pragma once

#include "Bloom/Application.hpp"

#include "Poppy/UI/Dockspace.hpp"
#include "Poppy/UI/ImGuiContext.hpp"
#include "Poppy/UI/View.hpp"

#include <utl/vector.hpp>
#include <utl/messenger.hpp>

namespace poppy {
	
	
	
	
//	class EditorWindowDelegate: public bloom::WindowDelegate {
//	private:
//		void init() override;
//		void shutdown() override;
//		void frame() override;
//
//	private:
//		void loadViews();
//
//		void populateView(View&);
//
//		void clearClosingViews();
//
//	private:
//		ImGuiContext imguiCtx;
//		Dockspace dockspace;
//		utl::vector<std::unique_ptr<View>> views;
//	};
//
	
	class EditorWindowDelegate: public bloom::WindowDelegate {
	private:
		void init() override;
		void shutdown() override;
		void frame() override;
		
	private:
		void loadViews();
		
		void populateView(View&);
		
		void clearClosingViews();
		
	private:
		
		ImGuiContext imguiCtx;
		Dockspace dockspace;
		utl::vector<std::unique_ptr<View>> views;
	};
	
	
	
	
	
}
