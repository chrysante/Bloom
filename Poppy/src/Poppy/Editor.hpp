#pragma once

#include "Bloom/Application/Application.hpp"
#include "Bloom/Scene/Scene.hpp"

#include "SelectionContext.hpp"
#include "ImGui/ImGui.hpp"
#include "Panels/Panel.hpp"
#include "ResourceManager.hpp"
#include "Debug.hpp"

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
		
	private:
		void init() override;
		void shutdown() override;
		void update(bloom::TimeStep) override;
		void render(bloom::TimeStep) override;
		void onEvent(bloom::Event&) override;
		
		void frame();
		void menuBar();
		
		template <typename>
		void createPanel(auto&&... args);
		template <typename>
		void menuItemForPanel(std::string_view name, bool unique, auto&&... args);
		Panel* findPanelByName(std::string_view name);
		
	private:
		void dockspace();
		void toolbar();
		
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
		float toolbarHeight = 50;
		
#if POPPY_DEBUGLEVEL > 0
		bool showImGuiDemo = false;
		bool showStyleColorsPanel = false;
#endif
	};
	
}
