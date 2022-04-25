#pragma once

#include <mtl/mtl.hpp>
#include <string>
#include <utl/vector.hpp>

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"
#include "Bloom/Core/Reference.hpp"
#include "Bloom/Graphics/StaticRenderMesh.hpp"
#include "Bloom/Graphics/Material.hpp"
#include "Bloom/Graphics/Lights.hpp"

#include "Entity.hpp"

namespace bloom {
	
	struct BLOOM_API TransformComponent {
		mtl::float3 position = 0;
		mtl::quaternion_float orientation = 1;
		mtl::float3 scale = 1;
		
		static TransformComponent fromMatrix(mtl::float4x4 const& m) {
			auto const [t, r, s] = mtl::decompose_transform(m);
			return { t, r, s };
		}
		
		[[ nodiscard ]] mtl::float4x4 calculate() const;
	};
	
	struct BLOOM_API TransformMatrixComponent {
		mtl::float4x4 matrix;
	};
	
	struct BLOOM_API TagComponent {
		std::string name;
	};
	
	struct BLOOM_API HierarchyComponent {
		EntityID parent, prevSibling, nextSibling, firstChild, lastChild;
	};
	
	struct BLOOM_API MeshRenderComponent {
		Reference<Material> material;
		Reference<StaticRenderMesh> mesh;
	};
	
	struct BLOOM_API LightComponent {
		LightComponent(): LightComponent(PointLight{}) {}
		LightComponent(PointLight p): light(p) {}
		LightComponent(SpotLight s): light(s) {}
		LightComponent(DirectionalLight d): light(d) {}
		
		LightType type() const { return (LightType)light.index(); }
		
		LightCommon& getCommon() { return utl::as_mutable(utl::as_const(*this).getCommon()); }
		LightCommon const& getCommon() const {
			switch (type()) {
				case LightType::pointlight: return get<PointLight>().common;
				case LightType::spotlight: return get<SpotLight>().common;
				case LightType::directional: return get<DirectionalLight>().common;
				default: bloomDebugbreak(); std::terminate();
			}
		}
		
		template <typename T>
		T& get() { return std::get<T>(light); }
		template <typename T>
		T const& get() const { return std::get<T>(light); }
		
	private:
		std::variant<PointLight, SpotLight, DirectionalLight> light;
	};
	
}
