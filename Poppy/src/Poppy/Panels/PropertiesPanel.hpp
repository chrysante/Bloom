#pragma once

#include <string>

namespace poppy {
	
	class PropertiesPanel {
	protected:
		void header(std::string_view name);
		bool beginSection();
		void endSection();
		
		void beginProperty(std::string_view label);
		
	private:
		
	};
	
}
