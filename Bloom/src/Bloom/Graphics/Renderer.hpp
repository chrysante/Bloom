#pragma once

#include "Bloom/Core/Base.hpp"

#include <mtl/mtl.hpp>
#include <utl/structure_of_arrays.hpp>
#include <utl/vector.hpp>
#include <utl/hashmap.hpp>
#include <string_view>
#include <iosfwd>
#include <array>

#include "Camera.hpp"
#include "RenderPrimitives.hpp"
#include "SceneRenderData.hpp"
#include "Lights.hpp"
#include "EntityRenderData.hpp"


namespace bloom {
	
	class FrameBuffer;
	class EditorFrameBuffer;
	class Camera;
	class Material;
	class StaticRenderMesh;
	class RenderContext;
	class RenderPassHandle;
	
	struct DebugDrawOptions {
		enum class Mode {
			lit = 0, wireframe, _count
		};
		Mode mode = Mode::lit;
	
		bool visualizeShadowCascades = false;
		/// used only with 'visualizeShadowCascades' == true
		uint32_t lightEntityID;
		
		bool alternateLightFrustum = false;
	};
	
	BLOOM_API std::string_view toString(DebugDrawOptions::Mode);
	BLOOM_API std::ostream& operator<<(std::ostream&, DebugDrawOptions::Mode);
	
	UTL_SOA_TYPE(SceneRenderObject,
				 (EntityRenderData, entity),
				 (bool, selected),
				 (Material*, material),
				 (StaticRenderMesh*, mesh));
	
//	struct ShadowMap {
//		TextureHandle cascades;
//		int numCascades = 0;
//		mtl::uint2 resolution = 0;
//	};
	
//	using LightSpaceTransformArray = std::array<mtl::float4x4, maxShadowCascades>;
//
//	UTL_SOA_TYPE(ShadowMapPass,
//				 (uint32_t, shadowMapID),
//				 (int, numCascades),
//				 (LightSpaceTransformArray, lightSpaceTransform)
//				 );
	
	class BLOOM_API Renderer {
	public:
		void init(RenderContext*);
		
		void beginScene(Camera const&, DebugDrawOptions);
		void endScene();
		
		void submit(StaticRenderMesh*, Material*, EntityRenderData, bool selected);
		
		void submit(PointLight, mtl::float3 position);
		void submit(SpotLight, mtl::float3 position, mtl::float3 direction);
		void submit(uint32_t id, DirectionalLight, mtl::float3 direction);
		
		void draw(FrameBuffer*);
		
		void debugDraw(EditorFrameBuffer*);
		
		RenderContext* getRenderContext() { return renderContext; }
#warning
		TextureView debugGetShadowMap(uint32_t id, int cascade) { return {}; }
		
		mtl::uint2 getShadowMapResolution() const { return shadowMapResolution; }
		void setShadowMapResolution(mtl::uint2 r) { shadowMapResolution = r; }
		
	private:
		void postprocess(EditorFrameBuffer*);
		
		RenderPassHandle mainPassEditor(EditorFrameBuffer*, DebugDrawOptions::Mode);
		RenderPassHandle shadowMapPass();
		RenderPassHandle outlinePass(EditorFrameBuffer*);
		RenderPassHandle editorPP(EditorFrameBuffer*, bool visShadowCascades, uint32_t lightEntity);
		
	private:
		void uploadEntityData();
		void uploadSceneData();
		void uploadDebugDrawData();
		
		void uploadShadowData(ShadowRenderData const&);
		void createShadowPipeline();
		void createShadowMapSampler();
		
		void createPostprocessQuad();
		void createPostprocessPipelines();
		void createPostprocessSampler();
		
		[[ nodiscard ]] TextureHandle createShadowMaps(int totalShadowMaps);
		
	private:
		bool buildingScene = false;
		RenderContext* renderContext = nullptr;
		
		DebugDrawOptions options;
		
		/// CPU Side Scene Data
		/// This must be flushed on beginScene()
		Camera camera;
		utl::structure_of_arrays<SceneRenderObject> objects;
		utl::vector<RenderPointLight> pointLights;
		utl::vector<RenderSpotLight> spotLights;
		utl::vector<RenderDirectionalLight> dirLights;
		
		/// Shadow Stuff
//		utl::hashmap<uint32_t, ShadowMap> shadowMaps;
//		utl::structure_of_arrays<ShadowMapPass> shadowMapPassses;
		RenderPipelineHandle shadowPipeline;
		BufferHandle shadowDataBuffer;
		SamplerHandle shadowMapSampler;
		
		
		
		int numShadowCasters;
		utl::small_vector<int> numCascades;
		utl::vector<mtl::float4x4> lightSpaceTransforms;
		TextureHandle shadowMapArray;
		int shadowMapArrayLength = 0;
		mtl::uint2 shadowMapResolution = 512;
		int visualizerIndex = 0;
		
		
		/// Main Pass Render Buffers
		BufferHandle entityDataBuffer, sceneDataBuffer, debugDrawDataBuffer;
		DepthStencilHandle depthStencil;
		
		
		/// Post Processing
		BufferHandle quadVB, quadIB;
		RenderPipelineHandle postprocessPipeline;
		RenderPipelineHandle editorPPPipeline;
		SamplerHandle postprocessSampler;
	};
	
}
