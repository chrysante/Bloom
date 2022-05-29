#pragma once

namespace poppy {
	
	class DebugViews {
	public:
		void display();
		void menubar();
		
		bool showDemoWindow = false;
		bool showUIDebugger = false;
		bool showStylePanel = false;
	};
	
}
