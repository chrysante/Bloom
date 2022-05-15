#pragma once

#include "Asset.hpp"

#include <utl/vector.hpp>
#include <string>

namespace bloom {

	BLOOM_API utl::vector<std::string> findClassNames(std::string_view script);
	
	class BLOOM_API ScriptAsset: public Asset {
	public:
		using Asset::Asset;
		
		void setText(std::string);
		
		std::string text;
		utl::vector<std::string> classes;
	};


}
