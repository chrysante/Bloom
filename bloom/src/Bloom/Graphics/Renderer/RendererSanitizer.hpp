#include "Bloom/Core/Core.hpp"

namespace bloom {
	
	class RendererSanitizer {
	public:
		void init() { isInit = true; }
		void beginScene() { bloomExpect(isInit); buildingScene = true;  }
		void endScene() { bloomExpect(isInit); buildingScene = false; }
		void submit() { bloomExpect(isInit); bloomExpect(buildingScene); }

	private:
		bool buildingScene = false;
		bool isInit = false;
	};
	
}

