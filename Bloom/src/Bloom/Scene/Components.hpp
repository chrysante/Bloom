#pragma once

#include <mtl/mtl.hpp>
#include <string>
#include <utl/vector.hpp>

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"
#include "Bloom/Graphics/StaticMesh.hpp"
#include "Bloom/Graphics/Material.hpp"
#include "Bloom/Graphics/Lights.hpp"

#include "Entity.hpp"

namespace bloom {
	
	struct BLOOM_API TransformComponent {
		mtl::float3 position = 0;
		mtl::quaternion_float orientation = 1;
		mtl::float3 scale = 1;
		
		[[ nodiscard ]] mtl::float4x4 calculate() const;
	};
	
	struct BLOOM_API TagComponent {
		std::string name;
	};
	
	struct BLOOM_API ParentComponent {
		EntityID entity;
	};
	
	struct BLOOM_API ChildrenComponent {
		utl::small_vector<EntityID> entities;
	};
	
	struct BLOOM_API MeshRenderComponent {
		Reference<Material> material;
		Reference<StaticMesh> mesh;
	};
	
	struct BLOOM_API LightComponent {
		LightComponent(PointLight p): light(p) {}
		LightComponent(SpotLight s): light(s) {}
		
		LightType type() const { return (LightType)light.index(); }
		
		template <typename T>
		T& get() { return std::get<T>(light); }
		template <typename T>
		T const& get() const { return std::get<T>(light); }
		
	private:
		std::variant<PointLight, SpotLight> light;
	};
	
}
