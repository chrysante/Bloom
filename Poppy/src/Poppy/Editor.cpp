#include "Editor.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>
#include <filesystem>
#include <utl/stdio.hpp>
#include <fstream>
#include <sstream>

#include "Bloom/Application/Resource.hpp"
#include "Bloom/Scene/SceneSystem.hpp"
#include "Bloom/Scene/SceneSerialize.hpp"
#include "Bloom/Assets/AssetManager.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"

#include "Panels/Viewport/Viewport.hpp"
#include "Panels/RendererSettingsPanel.hpp"
#include "Panels/SceneInspector.hpp"
#include "Panels/EntityInspector.hpp"
#include "Panels/AssetBrowser.hpp"

#include "IconConfig.hpp"

using namespace mtl::short_types;
using namespace bloom;

bloom::Application* createBloomApplication() {
	return new poppy::Editor();
}

namespace {
	
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
		loadSettingsFromINI();
		
		resourceManager.setRenderContext(&renderContext());
		imgui.init(&renderContext());
		GImGui->Style.WindowRounding = 5;
		GImGui->Style.FrameRounding = 5;
		
		LoadStyleColors(settings["ImGui Style Colors"]);
		
		createPanel<Viewport>(&renderer());
		createPanel<RendererSettingsPanel>(&renderer());
		createPanel<SceneInspector>();
		createPanel<EntityInspector>();
		createPanel<AssetBrowser>();
	}
	
	void Editor::shutdown() {
		SaveStyleColors(settings["ImGui Style Colors"]);
		for (auto& panel: panels) {
			panel->shutdown();
			settings[yamlSanitize(panel->uniqueName())] = panel->settings;
		}
		saveSettingsToINI();
	}
	
	void Editor::update(bloom::TimeStep) {
		
	}
	
	void Editor::render(bloom::TimeStep) {
		imgui.beginFrame();
		frame();
		imgui.present();
	}
	
	void Editor::onInputEvent(bloom::InputEvent& event) {
		imgui.handleMouseEvent(event);
		
		event.dispatch<InputEventType::keyUp>([&](KeyEvent e){
			if (e.key == Key::S && test(e.modifierFlags & InputModifierFlags::super)) {
				saveScene();
				return true;
			}
			return false;
		});
		
		if (event.handled()) {
			return;
		}
		
		auto const focusedPanel = std::find_if(panels.begin(), panels.end(), [](auto const& p) { return p->focused(); });
		if (focusedPanel != panels.end()) {
			(**focusedPanel).onInputEvent(event);
			if (event.handled()) {
				return;
			}
		}
		// give to all unfocused panels
		for (auto& panel: panels) {
			if (panel->focused()) {
				continue;
			}
			panel->onInputEvent(event);
			if (event.handled()) {
				break;
			}
		}
	}
	
	void Editor::frame() {
		menuBar();
		
		dockspace();
		toolbar();
	
		for (auto p = panels.begin(); p < panels.end();) {
			auto& panel = *p;
			panel->doDisplay();
			if (panel->shouldClose()) {
				p = panels.erase(p);
			}
			else {
				++p;
			}
		}
		
#if POPPY_DEBUGLEVEL > 0
		if (showImGuiDemo) {
			ImGui::ShowDemoWindow(&showImGuiDemo);
		}
		if (showStyleColorsPanel) {
			StyleColorsPanel(&showStyleColorsPanel);
		}
		if (showAlternateStyleColorsPanel) {
			AlternateStyleColorsPanel(&showAlternateStyleColorsPanel);
		}
#endif
	}
	
	void Editor::menuBar() {
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Scene...")) {
				newScene();
			}
			if (ImGui::MenuItem("Save Scene")) {
				saveScene();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Views")) {
			menuItemForPanel<Viewport>("Viewport", false, &renderer());
			menuItemForPanel<RendererSettingsPanel>("Renderer Settings", false, &renderer());
			
			menuItemForPanel<AssetBrowser>("Asset Browser", false);
			menuItemForPanel<SceneInspector>("Scene Inspector", true);
			menuItemForPanel<EntityInspector>("Entity Inspector", true);
			
			
			ImGui::EndMenu();
		}
		
#if POPPY_DEBUGLEVEL > 0
		if (ImGui::BeginMenu("Debug")) {
			if (ImGui::MenuItem("Demo Window")) {
				showImGuiDemo = true;
			}
			if (ImGui::MenuItem("Style Colors Panel")) {
				showStyleColorsPanel = true;
			}
			ImGui::EndMenu();
		}
#endif
		
		ImGui::EndMainMenuBar();
	}
	
	template <typename T>
	void Editor::createPanel(auto&&... args) {
		auto panel = Panel::create<T>(UTL_FORWARD(args)...);
		panel->settings = settings[yamlSanitize(panel->uniqueName())];
		panel->init();
		panels.push_back(std::move(panel));
	}
	
	template <typename T>
	void Editor::menuItemForPanel(std::string_view name, bool unique, auto&&... args) {
		if (unique) {
			if (ImGui::MenuItem(name.data())) {
				auto* const p = findPanelByName(name);
				if (p) {
					p->setFocused();
				}
				else {
					createPanel<T>(UTL_FORWARD(args)...);
				}
			}
		}
		else {
			if (ImGui::BeginMenu(name.data())) {
				auto* const p = findPanelByName(name);
				if (p && ImGui::MenuItem("Focus")) {
					p->setFocused();
				}
				if (ImGui::MenuItem("Add")) {
					createPanel<T>(UTL_FORWARD(args)...);
				}
				ImGui::EndMenu();
			}
		}
	}
	
	
	void Editor::dockspace() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		auto nextWindowPos = viewport->Pos;
		auto nextWindowSize = viewport->Size;
#if 1
		nextWindowPos.y += toolbarHeight;
		nextWindowSize.y -= toolbarHeight;
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
		
		int dsFlags = 0;
		dsFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
		dsFlags |= ImGuiDockNodeFlags_NoCloseButton;
		ImGui::DockSpace(dockMain, /* size arg */ {}, dsFlags);
		style.WindowMinSize.x = winSizeSaved;
		ImGui::End();
		ImGui::PopStyleVar(3);
	}
	
	void Editor::toolbar() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x,
									   viewport->Pos.y + ImGui::FindWindowByName("Master DockSpace")->MenuBarHeight()));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));
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
		ImGui::PushStyleColor(ImGuiCol_WindowBg, GImGui->Style.Colors[ImGuiCol_TitleBg]);
		ImGui::Begin("TOOLBAR", NULL, window_flags);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		auto& sceneSystem = this->sceneSystem();
		bool const isSimulating = sceneSystem.isSimulating();
		
		
		
		ImGui::BeginDisabled(sceneSystem.getScene() == nullptr);
		auto const buttonLabel = !isSimulating ? IconConfig::unicodeStr("play") : IconConfig::unicodeStr("stop");
		ImGui::PushFont((ImFont*)IconConfig::font(16));
		bool const playStop = ImGui::Button(buttonLabel.data(), ImVec2(37, 37));
		ImGui::PopFont();
		if (playStop) {
			!isSimulating ? startSimulation() : stopSimulation();
		}
		
		ImGui::EndDisabled();
		
		ImGui::SameLine();
		if (ImGui::Button("Some Other Button", ImVec2(0, 37))) {
			
		}
		ImGui::Separator();
		
		ImGui::End();
	}
	
	void Editor::startSimulation() {
		sceneBackup = scene->scene.copy();
		sceneSystem().runUpdateThread();
	}
	
	void Editor::stopSimulation() {
		sceneSystem().stopUpdateThread();
		scene->scene = std::move(sceneBackup);
	}
	
	void Editor::initLibraryDir() {
		auto const systemLibDir = bloom::getLibraryDir();
		libDir = systemLibDir / "Poppy";
		if (!std::filesystem::exists(libDir)) {
			std::filesystem::create_directory(libDir);
		}
	}
	
	void Editor::loadSettingsFromINI() {
		auto const path = libraryDir() / "poppy.ini";
		std::fstream settingsFile(path, std::ios::in);
		if (!settingsFile) {
			poppyLog(warning, "Failed to load Settings from {}", path);
			return;
		}
		std::stringstream sstr;
		sstr << settingsFile.rdbuf();
		settings = YAML::Load(sstr.str());
	}
	
	void Editor::saveSettingsToINI() {
		auto const path = libraryDir() / "poppy.ini";
		std::fstream settingsFile(path, std::ios::out);
		if (!settingsFile) {
			poppyLog(warning, "Failed to Open Settings File at {}", path);
			return;
		}
		YAML::Emitter out;
		out << settings;
		settingsFile << out.c_str();
	}
	
	void Editor::newScene() {
		showSaveFilePanel([this](std::filesystem::path path) {
			auto asset = assetManager().create(AssetType::scene,
											   path.filename().replace_extension().string(),
											   std::filesystem::path{ path }.remove_filename());
			setScene(as<SceneAsset>(asset));
		});
	}
	
	void Editor::saveScene() {
		assetManager().saveToDisk(scene->handle());
	}
	
	void Editor::setScene(Reference<SceneAsset> scene) {
		this->scene = std::move(scene);
		selection.clear();
		sceneSystem().setScene(bloom::Reference<Scene>(this->scene, &this->scene->scene));
	}
	
	std::unique_ptr<bloom::AssetManager> Editor::createAssetManager() {
		return std::make_unique<AssetManager>();
	}
	
	Panel* Editor::findPanelByName(std::string_view name) {
		for (auto& panel: panels) {
			if (panel->matchesName(name)) {
				return panel.get();
			}
		}
		return nullptr;
	}
	
}
