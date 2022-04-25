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
	public:
		Editor();
		
		std::filesystem::path libraryDir() const { return libDir; }
		
		ResourceManager* getResourceManager() { return &resourceManager; }
		
	private:
		void init() override;
		void shutdown() override;
		void update(bloom::TimeStep) override;
		void render(bloom::TimeStep) override;
		void onEvent(bloom::Event const&) override;
		
		void frame();
		void menuBar();
		
		template <typename>
		void createPanel(auto&&... args);
		template <typename>
		void menuItemForPanel(std::string_view name, bool unique, auto&&... args);
		Panel* findPanelByName(std::string_view name);
		
		
	private:
		void initLibraryDir();
		void loadSettingsFromINI();
		void saveSettingsToINI();
		
		std::unique_ptr<bloom::AssetManager> createAssetManager() override;
		
		
		
	private:
		ImGuiContext imgui;
		utl::ref<bloom::Scene> scene;
		SelectionContext selection;
		utl::vector<utl::unique_ref<Panel>> panels;
		std::filesystem::path libDir;
		YAML::Node settings;
		ResourceManager resourceManager;
		
#if POPPY_DEBUGLEVEL > 0
		bool showImGuiDemo = false;
		bool showStyleColorsPanel = false;
#endif
	};
	
}
