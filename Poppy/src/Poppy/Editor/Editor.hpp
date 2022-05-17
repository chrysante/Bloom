#pragma once

#include "Dockspace.hpp"

#include "Bloom/Application/Application.hpp"
#include "Bloom/Scene/Scene.hpp"

#include "Poppy/SelectionContext.hpp"
#include "Poppy/ImGui/ImGui.hpp"
#include "Poppy/Panels/Panel.hpp"
#include "Poppy/ResourceManager.hpp"
#include "Poppy/Debug.hpp"

#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <utl/vector.hpp>

namespace poppy {
	
	class Editor: public bloom::Application {
		friend class BasicSceneInspector;
		
	public:
		Editor();
		
		std::filesystem::path libraryDir() const { return libDir; }
		
		ResourceManager* getResourceManager() { return &resourceManager; }
		void setScene(bloom::Reference<bloom::SceneAsset>);
		
		static Editor& get() { return utl::down_cast<Editor&>(bloom::Application::get()); }
		
	private:
		void init() override;
		void shutdown() override;
		void update(bloom::TimeStep) override;
		void render(bloom::TimeStep) override;
		void onInputEvent(bloom::InputEvent&) override;
		
		void frame();
		void menuBar();
		
		template <typename>
		void createPanel(auto&&... args);
		template <typename>
		void menuItemForPanel(std::string_view name, bool unique, auto&&... args);
		Panel* findPanelByName(std::string_view name);
		
	public:
		void startSimulation();
		void stopSimulation();
		
	private:
		void initLibraryDir();
		void loadSettingsFromINI();
		void saveSettingsToINI();
		void newScene();
		void saveScene();
		
		std::unique_ptr<bloom::AssetManager> createAssetManager() override;
		
	private:
		ImGuiContext imgui;
		bloom::Reference<bloom::SceneAsset> scene;
		bloom::Scene sceneBackup;
		SelectionContext selection;
		utl::vector<utl::unique_ref<Panel>> panels;
		std::filesystem::path libDir;
		YAML::Node settings;
		ResourceManager resourceManager;
		
		Dockspace dockspace;
#if POPPY_DEBUGLEVEL > 0
		bool showImGuiDemo = false;
		bool showStyleColorsPanel = false;
		bool showUIDebugger = false;
#endif
	};
	
}
