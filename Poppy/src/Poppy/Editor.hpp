#pragma once

#include "Bloom/Application/Application.hpp"
#include "Bloom/Scene/Scene.hpp"

#include "SelectionContext.hpp"
#include "ImGui/ImGui.hpp"
#include "Panels/Panel.hpp"
#include "AppSettings.hpp"

#include <utl/vector.hpp>

namespace poppy {
	
	class Editor: public bloom::Application {
	public:
		Editor();
		
		AppSettings& settings() { return appSettings; }
		std::filesystem::path libraryDir() const { return libDir; }
		
	private:
		void init() override;
		void shutdown() override;
		void update(bloom::TimeStep) override;
		void render(bloom::TimeStep) override;
		void onEvent(bloom::Event const&) override;
		
		void frame();
		void menuBar();
		
	private:
		void initLibraryDir();
		
	private:
		ImGuiContext imgui;
		utl::ref<bloom::Scene> scene;
		SelectionContext selection;
		utl::vector<utl::unique_ref<Panel>> panels;
		AppSettingsMaster appSettings;
		std::filesystem::path libDir;
	};
	
}
