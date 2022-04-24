#include "Editor.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>
#include <filesystem>
#include <utl/stdio.hpp>
#include <fstream>
#include <sstream>

#include "Bloom/Application/Resource.hpp"

#include "Debug.hpp"
#include "ExampleScene.hpp"
#include "EditorAssetManager.hpp"

#include "Panels/Viewport.hpp"
#include "Panels/SceneInspector.hpp"
#include "Panels/EntityInspector.hpp"
#include "Panels/AssetBrowser.hpp"


using namespace mtl::short_types;

bloom::Application* createBloomApplication() {
	return new poppy::Editor();
}

namespace {
	const float toolbarSize = 50;
	
	static void DockSpaceUI() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		auto nextWindowPos = viewport->Pos;
		auto nextWindowSize = viewport->Size;
#if 1
		nextWindowPos.y += toolbarSize;
		nextWindowSize.y -= toolbarSize;
#endif
		ImGui::SetNextWindowPos(nextWindowPos);
		ImGui::SetNextWindowSize(nextWindowSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGuiWindowFlags window_flags = 0
			| ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("Master DockSpace", NULL, window_flags);
		ImGuiID dockMain = ImGui::GetID("MyDockspace");

	//	ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto const winSizeSaved = style.WindowMinSize.x;
		style.WindowMinSize.x = 250;
		ImGui::DockSpace(dockMain);
		style.WindowMinSize.x = winSizeSaved;
		ImGui::End();
		ImGui::PopStyleVar(3);
	}

	static void ToolbarUI() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x,
									   viewport->Pos.y + ImGui::FindWindowByName("Master DockSpace")-> MenuBarHeight()));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarSize));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags = 0
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoSavedSettings
			;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::Begin("TOOLBAR", NULL, window_flags);
		ImGui::PopStyleVar();


		ImGui::Button("Toolbar goes here", ImVec2(0, 37)); ImGui::SameLine();
		ImGui::Button("Toolbar goes here", ImVec2(0, 37)); ImGui::SameLine();
		ImGui::Button("Toolbar goes here", ImVec2(0, 37)); ImGui::SameLine();



		ImGui::End();
	}

	std::string yamlSanitize(std::string s) {
		std::replace(s.begin(), s.end(), '#', '*');
		return std::move(s);
	}
	
	std::string yamlSanitize(std::string_view s) {
		return yamlSanitize(std::string(s));
	}
	
}

namespace poppy {
	
	Editor::Editor() {
	
	}
	
	void Editor::init() {
		initLibraryDir();
		
		do {
			auto const path = libraryDir() / "poppy.ini";
			std::fstream settingsFile(path, std::ios::in);
			if (!settingsFile) {
				poppyLog(warning, "Failed to load Settings from {}", path);
				break;
			}
			std::stringstream sstr;
			sstr << settingsFile.rdbuf();
			settings = YAML::Load(sstr.str());
		} while (0);
		resourceManager.setRenderContext(&renderContext());
		imgui.init(&renderContext());
		GImGui->Style.WindowRounding = 5;
		GImGui->Style.FrameRounding = 5;
		
		scene = utl::make_ref<bloom::Scene>();
		buildExampleScene(*scene, renderContext());
		
		createPanel<Viewport>(&renderer());
		createPanel<Viewport>(&renderer());
		createPanel<SceneInspector>();
		createPanel<EntityInspector>();
		createPanel<AssetBrowser>();
	}
	
	void Editor::shutdown() {
		for (auto& panel: panels) {
			panel->shutdown();
			settings[yamlSanitize(panel->uniqueName())] = panel->settings;
		}
		do {
			auto const path = libraryDir() / "poppy.ini";
			std::fstream settingsFile(path, std::ios::out);
			if (!settingsFile) {
				poppyLog(warning, "Failed to Open Settings File at {}", path);
				break;
			}
			YAML::Emitter out;
			out << settings;
			settingsFile << out.c_str();
		} while (0);
	}
	
	void Editor::update(bloom::TimeStep) {
		
	}
	
	void Editor::render(bloom::TimeStep) {
		imgui.beginFrame();
		frame();
		imgui.present();
	}
	
	void Editor::onEvent(bloom::Event const& event) {
		imgui.handleMouseEvent(event);
		bool eventHandled = false;
		for (auto& panel: panels) {
			if (!eventHandled && panel->focused()) {
				if (!(panel->_ignoreEventMask & event.type())) {
					panel->onEvent(event);
				}
				eventHandled = true;
			}
		}
	}
	
	void Editor::frame() {
		menuBar();
		
		DockSpaceUI();
		ToolbarUI();
	
		for (auto& panel: panels) {
			panel->doDisplay();
		}
	}
	
	void Editor::menuBar() {
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("Test")) {
			ImGui::MenuItem("One");
			ImGui::MenuItem("Two");
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	
	template <typename T>
	void Editor::createPanel(auto&&... args) {
		auto panel = Panel::create<T>(this, UTL_FORWARD(args)...);
		if (auto* const sc = dynamic_cast<BasicSceneInspector*>(panel.get())) {
			sc->setScene(scene.get());
			sc->setSelectionContext(&selection);
			sc->setAssetManager(getAssetManager());
		}
		
		panel->settings = settings[yamlSanitize(panel->uniqueName())];
		panel->init();
		panels.push_back(std::move(panel));
	}
	
	void Editor::initLibraryDir() {
		auto const systemLibDir = bloom::getLibraryDir();
		libDir = systemLibDir / "Poppy";
		if (!std::filesystem::exists(libDir)) {
			std::filesystem::create_directory(libDir);
		}
	}
	
	std::unique_ptr<bloom::AssetManager> Editor::createAssetManager() {
		return std::make_unique<EditorAssetManager>();
	}
	
}
