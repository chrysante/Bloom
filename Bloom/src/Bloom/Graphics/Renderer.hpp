#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"

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
		uint32_t lightVizEntityID = 0xFFffFFff;
		
		bool alternateLightFrustum = false;
	};
	
	BLOOM_API std::string_view toString(DebugDrawOptions::Mode);
	BLOOM_API std::ostream& operator<<(std::ostream&, DebugDrawOptions::Mode);
	
	UTL_SOA_TYPE(SceneRenderObject,
				 (EntityRenderData, entity),
				 (Reference<Material>, material),
				 (Reference<StaticRenderMesh>, mesh));
	
	class BLOOM_API Renderer {
	public:
		void init(RenderContext*);
		
		void beginScene(Camera const&, DebugDrawOptions);
		void endScene();
		
		void submit(Reference<StaticRenderMesh>, Reference<Material>, EntityRenderData);
		
		void submitSelected(Reference<StaticRenderMesh>, Reference<Material>, EntityRenderData);
		
		void submit(PointLight);
		void submit(SpotLight);
		void submit(DirectionalLight);
		void submit(SkyLight);
		
		void submitShadowCascadeViz(DirectionalLight);
		
		void draw(FrameBuffer*);
		void drawDebugInfo(EditorFrameBuffer*);
		
		RenderContext* getRenderContext() { return renderContext; }
		
		mtl::uint2 getShadowMapResolution() const { return shadowMapResolution; }
		void setShadowMapResolution(mtl::uint2);
		
		TriangleCullMode getShadowCullMode() const { return shadowCullMode; }
		void setShadowCullMode(TriangleCullMode mode) { shadowCullMode = mode; }
		
	private:
		void postprocess(FrameBuffer*);
		
		RenderPassHandle mainPass(FrameBuffer*);
		RenderPassHandle wireframePass(FrameBuffer*);
		RenderPassHandle editorPass(EditorFrameBuffer*, DebugDrawOptions::Mode);
		RenderPassHandle shadowMapPass();
		RenderPassHandle selectionPass(EditorFrameBuffer*);
		RenderPassHandle editorPP(EditorFrameBuffer*, bool visShadowCascades, uint32_t lightEntity);
		
	private:
		void uploadEntityData();
		void uploadSceneData();
		void uploadDebugDrawData();
		
		void uploadShadowData();
		void createShadowPipeline();
		void createShadowMapSampler();
		
		void createEditorPassPipeline();
		void createWireframePassPipeline();
		
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
		utl::vector<PointLight> pointLights;
		utl::vector<SpotLight> spotLights;
		utl::vector<DirectionalLight> dirLights;
		utl::vector<SkyLight> skyLights;
		
		utl::structure_of_arrays<SceneRenderObject> selectedObjects;
		
		/// Shadow Stuff
		RenderPipelineHandle shadowPipeline;
		BufferHandle shadowDataBuffer;
		SamplerHandle shadowMapSampler;
		
		int numShadowCasters;
		utl::small_vector<int> numCascades;
		utl::vector<mtl::float4x4> lightSpaceTransforms;
		TextureHandle shadowMapArray;
		int shadowMapArrayLength = 0;
		mtl::uint2 shadowMapResolution = 512;
		bool needsNewShadowMaps = true;
		TriangleCullMode shadowCullMode = TriangleCullMode::front;
	
		uint shadowCascadeVizCount = 0;
		std::array<mtl::float4x4, 10> shadowCascadeVizTransforms{};
		
		/// Main Pass Render Buffers
		BufferHandle entityDataBuffer, selectedEntityDataBuffer, sceneDataBuffer, debugDrawDataBuffer;
		DepthStencilHandle depthStencil;
		
		/// Editor Pass
		RenderPipelineHandle editorPassPipeline;
		RenderPipelineHandle wireframePassPipeline;
		
		/// Post Processing
		BufferHandle quadVB, quadIB;
		RenderPipelineHandle postprocessPipeline;
		RenderPipelineHandle editorPPPipeline;
		SamplerHandle postprocessSampler;
	};
	
}
