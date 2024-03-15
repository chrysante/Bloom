#if 0

#include "EditorWindowDelegate.hpp"

#include "Editor.hpp"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/ImGuiHelpers.h"

#include "Bloom/Core.h"
#include "Bloom/GPU.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace bloom;

namespace poppy {
	
	struct TestView: View {
		TestView(): View("Test View") {}
		
		void frame() override {
			displayEmptyWithReason("Nothing to see here");
			return;
			ImGui::Text("%s", utl::format("View Size:     {}", size()).data());
			ImGui::Text("%s", utl::format("View Position: {}", position()).data());
			ImGui::Text("%s", utl::format("Window Size:   {}", windowSize()).data());
		}
	};
	
	void EditorWindowDelegate::init() {
		ImGuiContextDescription desc;
		desc.iniFilePath = bloom::libraryDir() / "Poppy/imgui.ini";
		imguiCtx.init(window().application().device(), desc);
		
		window().onInput([this](InputEvent const& e) {
			imguiCtx.onInput(e);
		});
		window().onCharInput([this](unsigned int code) {
			imguiCtx.onCharInput(code);
		});
		
		window().createDefaultSwapchain(window().application().device());
		window().setCommandQueue(window().application().device().createCommandQueue());
		
		dockspace.setCenterToolbar({
			ToolbarIconButton("menu"),
			ToolbarButton("Button")
		});
		
		loadViews();
	}
	
	void EditorWindowDelegate::shutdown() {
		imguiCtx.shutdown();
	}
	
	void EditorWindowDelegate::frame() {
		clearClosingViews();
		
		imguiCtx.newFrame(window());
		utl::down_cast<Editor&>(application()).menuBar();
		dockspace.display();
		
		for (auto& view: views) {
			view->doFrame();
		}
		
		imguiCtx.drawFrame(window().application().device(), window());
	}
	
	void EditorWindowDelegate::loadViews() {
		auto view = std::unique_ptr<View>(new TestView());
		populateView(*view);
		view->doInit();
		views.push_back(std::move(view));
	}
	
	void EditorWindowDelegate::populateView(View& view) {
		auto& desc = view.desc;
		desc.editor = utl::down_cast<Editor*>(&window().application());
	}
	
	void EditorWindowDelegate::clearClosingViews() {
		for (auto itr = views.begin(); itr != views.end();) {
			auto& view = *itr;
			
			if (view->shouldClose()) {
				view->doShutdown();
				itr = views.erase(itr);
			}
			else {
				++itr;
			}
		}
	}
	
}

#endif
