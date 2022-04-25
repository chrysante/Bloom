#pragma once

#include "Bloom/Core/Base.hpp"

#include <mtl/mtl.hpp>
#include <utl/structure_of_arrays.hpp>
#include <utl/vector.hpp>
#include <string_view>
#include <iosfwd>

#include "Camera.hpp"
#include "RenderPrimitives.hpp"
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
	
	enum class DebugDrawMode {
		lit = 0, wireframe, _count
	};
	
	BLOOM_API std::string_view toString(DebugDrawMode);
	BLOOM_API std::ostream& operator<<(std::ostream&, DebugDrawMode);
	
	UTL_SOA_TYPE(SceneRenderObject,
				 (EntityRenderData, entity),
				 (bool, selected),
				 (Material*, material),
				 (StaticRenderMesh*, mesh));
	
	class BLOOM_API Renderer {
	public:
		void init(RenderContext*);
		
		void beginScene(Camera const&);
		void endScene();
		
		void submit(StaticRenderMesh*, Material*, EntityRenderData, bool selected);
		
		void submit(PointLight, mtl::float3 position);
		void submit(SpotLight, mtl::float3 position, mtl::float3 direction);
		void submit(DirectionalLight, mtl::float3 direction);
		
		void draw(FrameBuffer*);
		
		void debugDraw(EditorFrameBuffer*, DebugDrawMode);
		
		RenderContext* getRenderContext() { return renderContext; }
		
	private:
		void postprocess(EditorFrameBuffer*);
		
		RenderPassHandle mainPassEditor(EditorFrameBuffer*, DebugDrawMode);
		RenderPassHandle outlinePass(EditorFrameBuffer*);
		RenderPassHandle editorPP(EditorFrameBuffer*);
		
	private:
		void createPostprocessQuad();
		void createPostprocessPipelines();
		void createPostprocessSampler();
		
		void uploadEntityData();
		
	private:
		bool buildingScene = false;
		RenderContext* renderContext = nullptr;
		
		Camera camera;
		utl::structure_of_arrays<SceneRenderObject> objects;
		utl::vector<RenderPointLight> pointLights;
		utl::vector<RenderSpotLight> spotLights;
		utl::vector<RenderDirectionalLight> dirLights;
		
		BufferHandle entityDataBuffer, sceneDataBuffer;
		DepthStencilHandle depthStencil;
		
		BufferHandle quadVB, quadIB;
		
		RenderPipelineHandle postprocessPipeline;
		
		RenderPipelineHandle editorPPPipeline;
		
		SamplerHandle postprocessSampler;
		
		
	};
	
}
