#include "View.hpp"

#include "Font.hpp"
#include "ImGuiHelpers.hpp"

#include <utl/format.hpp>
#include <utl/scope_guard.hpp>
#include <utl/hashset.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
 
using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
	
	static int gID = 0;
	static utl::hashset<int> usedIDs;
	
	static int getFreshID() {
		while (usedIDs.contains(gID)) {
			++gID;
		}
		return gID;
	}
	
	/// MARK: Initialization
	View::View(ViewDescription const& d): desc{ d } {
		
	}
	
	/// MARK: Queries
	bool View::focused() const {
		return GImGui->NavWindow == desc.imguiWindow;
	}
	
	mtl::float2 View::windowSpaceToViewSpace(mtl::float2 position) const {
		return position - desc.position;
	}
	
	mtl::float2 View::viewSpaceToWindowSpace(mtl::float2 position) const {
		return position + desc.position;
	}
	
	/// MARK: Modifiers
	void View::setFocused() {
		ImGui::FocusWindow((ImGuiWindow*)desc.imguiWindow);
	}
	
	void View::maximize() {
		desc.maximized = true;
		// send message to window
	}
	
	void View::restore() {
		desc.maximized = false;
		// send message to window
	}
	
	void View::toggleMaximize() {
		if (maximized()) {
			restore();
		}
		else {
			maximize();
		}
	}
	
	void View::setPadding(mtl::float2 padding) {
		desc.hasPaddingX = padding.x >= 0;
		desc.hasPaddingY = padding.y >= 0;
		
		desc.pub.padding = padding;
	}
	
	/// MARK: Convinience Helpers
	void View::displayEmptyWithReason(std::string_view reason) const {
		auto const oldCursorPos = ImGui::GetCursorPos();
		utl::scope_guard reset = [&]{
			ImGui::SetCursorPos(oldCursorPos);
		};
		
		Font font = Font::UIDefault();
//		font.size = FontSize::large;
		font.weight = FontWeight::semibold;
		
		withFont(font, [&]{
			float2 const textSize =	ImGui::CalcTextSize(reason.data());
			ImGui::SetCursorPos((size() - textSize) / 2);
			ImGui::TextDisabled("%s", reason.data());
		});
	}
	
	/// MARK: Private
	void View::doInit() {
		if (desc.pub.id < 0) {
			desc.pub.id = getFreshID();
		}
		usedIDs.insert(desc.pub.id);
		setPadding(desc.pub.padding);
		
		this->init();
	}
	
	void View::doShutdown() {
		this->shutdown();
	}
	
	void View::doFrame() {
		ImGuiWindowFlags flags = 0;
				
		flags |= ImGuiWindowFlags_NoCollapse;
		
		bool const maximized = desc.maximized;
		
		// Begin block
		if (maximized) {
			auto* const viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowPos({ 0, 0 });
			
			flags |= ImGuiWindowFlags_NoTitleBar;
			flags |= ImGuiWindowFlags_NoMove;
			flags |= ImGuiWindowFlags_NoResize;
			
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		}
		auto& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, desc.maximized ? 0 : style.WindowRounding);
		mtl::float2 const padding = {
			desc.hasPaddingX ? desc.pub.padding.x : style.WindowPadding.x,
			desc.hasPaddingY ? desc.pub.padding.y : style.WindowPadding.y
		};
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
		bool const open = ImGui::Begin((desc.maximized ?
										desc.pub.name + "_FS" :
										desc.pub.name).data(),
									   &desc.open,
									   flags);
		ImGui::PopStyleVar(2);
		
		if (maximized) { ImGui::PopStyleVar(); }
		
		if (open) {
			// gather view info
			auto* const window = ImGui::GetCurrentWindow();
			
			desc.imguiWindow = window;
			desc.pub.size = window->Size;
			desc.position = window->Pos;
			desc.windowSize = ImGui::GetMainViewport()->Size;
			
			this->frame();
		}
		
		ImGui::End();
	}
	
	void View::doOnInput(bloom::InputEvent&) {
		
	}
	
	YAML::Node View::doSerialize() const {
		YAML::Node node;
		node["View Base"] = desc.pub;
		node["Derived View"] = this->serialize(); // call to derived
		return node;
	}
	
	std::unique_ptr<View> View::doDeserialize(YAML::Node const& node) {
		try {
			ViewDescription const desc = node["View Base"].as<ViewDescription>();
			if (desc.id < 0) {
				poppyLog(error, "Negative View ID");
				poppyDebugbreak();
				return nullptr;
			}
			
			auto const factory = ViewRegistry::getFactory(desc.name);
			if (!factory) {
				poppyLog(error, "Failed to deserialize View named '{}'", desc.name);
				return nullptr;
			}
			
			auto result = factory();
			ViewDescPrivate privDesc{ desc };
			result->desc = privDesc;
			result->deserialize(node["Derived View"]);
			
			return result;
		}
		catch (std::exception const& e) {
			poppyLog(error, "{}", e.what());
		}
		
		return nullptr;
	}
	
}
