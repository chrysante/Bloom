#pragma once

#include "Poppy/UI/View.hpp"

#include "Bloom/Core/Core.hpp"

namespace bloom { class MaterialInstance; }

namespace poppy {
	
	class MaterialInstanceViewer: public View {
	public:
		void frame() override;
		
		void setMaterialInstance(bloom::Reference<bloom::MaterialInstance>);
	
	private:
		bloom::Reference<bloom::MaterialInstance> inst;
	};
	
}
