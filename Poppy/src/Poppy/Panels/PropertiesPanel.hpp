#pragma once

#include <string>
#include "Poppy/ImGui/ImGui.hpp"

namespace poppy {
	
	class PropertiesPanel {
	protected:
		void header(std::string_view name, FontWeight = FontWeight::semibold, FontStyle = FontStyle::roman);
		bool beginSection();
		void endSection();
		
		void beginProperty(std::string_view label);
		
		static bool dragFloat(std::string_view label, float* data, float speed = 1, float min = 0, float max = 0, char const* format = "%.3f");
		static bool sliderFloat(std::string_view label, float* data, float min, float max);
		static bool sliderInt(std::string_view label, int* data, int min, int max);
		
	private:
		
	};
	
}
