#pragma once

#include "Material.hpp"
#include "MaterialParameters.hpp"

#include "Bloom/Asset/Asset.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"

#include <yaml-cpp/yaml.h>

namespace bloom {
	
	class AssetManager;
	
	class BLOOM_API MaterialInstance: public Asset {
		friend class ForwardRenderer;
		friend class AssetManager;
		
	public:
		using Asset::Asset;
		
		Material* material() { return mMaterial.get(); }
		void setMaterial(Reference<Material>);
		BufferView parameterBuffer() const { return mParameterBuffer; }
		
		MaterialParameters const& paramaters() const { return mParameters; }
		void setParamaters(MaterialParameters const& params);
		
		YAML::Node serialize() const;
		void deserialize(YAML::Node const&, AssetManager&);
		
	private:
		Reference<Material> mMaterial;
		BufferHandle mParameterBuffer;
		MaterialParameters mParameters;
		bool mDirty = true;
	};
	
}
