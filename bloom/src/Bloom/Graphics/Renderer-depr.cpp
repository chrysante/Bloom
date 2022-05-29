#define UTL_DEFER_MACROS

#include "Renderer.hpp"

#include "Bloom/Core/Debug.hpp"

#include "Bloom/GPU/HardwareDevice.hpp"

#include "Framebuffer.hpp"
#include "Material.hpp"
#include "StaticRenderMesh.hpp"

#include <algorithm>
#include <numeric>
#include <array>
#include <utl/scope_guard.hpp>

using namespace mtl::short_types;

namespace bloom {
	
//	static mtl::float4x4 directionalLightSpaceTransform(Camera const& camera, float dist, mtl::float3 lightDirection) {
//		using namespace mtl;
//
//		float  const ar        = camera.aspectRatio();
//		float  const fov       = camera.fieldOfView();
//		float  const nearDist  = camera.nearClipPlane();
//		float  const farDist   = dist;
//		float  const Hnear     = 2 * std::tan(fov/2) * nearDist;
//		float  const Wnear     = Hnear * ar;
//		float  const Hfar      = 2 * std::tan(fov/2) * farDist;
//		float  const Wfar      = Hfar * ar;
//		float3 const centerFar = camera.position() + camera.front() * farDist;
//
//		float3 const topLeftFar     = centerFar + (camera.up() * Hfar / 2) - (camera.right() * Wfar / 2);
//		float3 const topRightFar    = centerFar + (camera.up() * Hfar / 2) + (camera.right() * Wfar / 2);
//		float3 const bottomLeftFar  = centerFar - (camera.up() * Hfar / 2) - (camera.right() * Wfar / 2);
//		float3 const bottomRightFar = centerFar - (camera.up() * Hfar / 2) + (camera.right() * Wfar / 2);
//
//		float3 const centerNear = camera.position() + camera.front() * nearDist;
//
//		float3 const topLeftNear     = centerNear + (camera.up() * Hnear / 2) - (camera.right() * Wnear / 2);
//		float3 const topRightNear    = centerNear + (camera.up() * Hnear / 2) + (camera.right() * Wnear / 2);
//		float3 const bottomLeftNear  = centerNear - (camera.up() * Hnear / 2) - (camera.right() * Wnear / 2);
//		float3 const bottomRightNear = centerNear - (camera.up() * Hnear / 2) + (camera.right() * Wnear / 2);
//
//		float3 const frustumCenter = (centerFar- centerNear) * 0.5f;
//
//		float4x4 const lightView = mtl::look_at<mtl::right_handed>(mtl::normalize(lightDirection), float3(0,0,0), float3(0,0,1));
//
//		std::array<float4, 8> const frustumToLightView {
//			lightView * float4(bottomRightNear, 1.0f),
//			lightView * float4(topRightNear,    1.0f),
//			lightView * float4(bottomLeftNear,  1.0f),
//			lightView * float4(topLeftNear,     1.0f),
//			lightView * float4(bottomRightFar,  1.0f),
//			lightView * float4(topRightFar,     1.0f),
//			lightView * float4(bottomLeftFar,   1.0f),
//			lightView * float4(topLeftFar,      1.0f)
//		};
//
//		// find max and min points to define a ortho matrix around
//		float3 min{  INFINITY,  INFINITY,  INFINITY };
//		float3 max{ -INFINITY, -INFINITY, -INFINITY };
//		for (std::size_t i = 0; i < frustumToLightView.size(); i++) {
//			if (frustumToLightView[i].x < min.x)
//				min.x = frustumToLightView[i].x;
//			if (frustumToLightView[i].y < min.y)
//				min.y = frustumToLightView[i].y;
//			if (frustumToLightView[i].z < min.z)
//				min.z = frustumToLightView[i].z;
//
//			if (frustumToLightView[i].x > max.x)
//				max.x = frustumToLightView[i].x;
//			if (frustumToLightView[i].y > max.y)
//				max.y = frustumToLightView[i].y;
//			if (frustumToLightView[i].z > max.z)
//				max.z = frustumToLightView[i].z;
//		}
//
//		float const l =  min.x;
//		float const r =  max.x;
//		float const b =  min.y;
//		float const t =  max.y;
//		// because max.z is positive and in NDC the positive z axis is
//		// towards us so need to set it as the near plane flipped same for min.z.
//		float const n = -max.z;
//		float const f = -min.z;
//
//		// finally, set our ortho projection
//		// and create the light space view-projection matrix
//		mtl::float4x4 const lightProjection = mtl::ortho<mtl::right_handed>(l,r,b,t,n,f);
//		mtl::float4x4 const lightSpaceMatrix = lightProjection * lightView;
//
//		return lightSpaceMatrix;
//	}

	static mtl::float4x4 directionalLightSpaceTransform(Camera const& camera, float dist, float zDist, mtl::float3 lightDirection) {
		mtl::float4x4 const lsProj = mtl::ortho<mtl::right_handed>(-dist, dist,
																   -dist, dist,
																   -zDist, zDist);

		mtl::float4x4 const lsView = mtl::look_at<mtl::right_handed>(camera.position(), camera.position() - lightDirection, { 0, 1, 0 });
		return lsProj * lsView;
	}
	
	
	void Renderer::init(HardwareDevice& device) {
		mDevice = &device;
		/* Buffers */ {
			BufferDescription desc{};
			desc.storageMode = StorageMode::managed;
			
			desc.size = sizeof(SceneRenderData);
			sceneDataBuffer = device.createBuffer(desc);
			
			desc.size = sizeof(DebugDrawData);
			debugDrawDataBuffer = device.createBuffer(desc);
		}
		{
			DepthStencilDescription desc{};
			desc.depthWrite = true;
			desc.depthCompareFunction = CompareFunction::lessEqual;
			
			depthStencil = device.createDepthStencil(desc);
		}
		
		createShadowPipeline();
		createShadowMapSampler();
		
		createEditorPassPipeline();
		createWireframePassPipeline();
		
		createPostprocessQuad();
		createPostprocessPipelines();
		createPostprocessSampler();
	}
	
	void Renderer::beginScene(Camera const& camera) {
		bloomExpect(!buildingScene, "Already called beginScene?");
		buildingScene = true;
//		this->options = options;
		
		this->camera = camera;
		objects.clear();
		pointLights.clear();
		spotLights.clear();
		dirLights.clear();
		skyLights.clear();
		
		// shadows
		numShadowCasters = 0;
		numCascades.clear();
		lightSpaceTransforms.clear();
		
		shadowCascadeVizCount = 0;
	}
	
	void Renderer::endScene() {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		buildingScene = false;
	}
	
	void Renderer::submit(Reference<StaticRenderMesh> mesh, Reference<Material> material, EntityRenderData entityData) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		entityData.transform = mtl::transpose(entityData.transform);
		objects.push_back({ entityData, std::move(material), std::move(mesh) });
	}
	
//	void Renderer::submitSelected(Reference<StaticRenderMesh> mesh, Reference<Material> material, EntityRenderData entityData) {
//		bloomExpect(buildingScene, "Forgot to call beginScene?");
//		entityData.transform = mtl::transpose(entityData.transform);
//		selectedObjects.push_back({ entityData, std::move(material), std::move(mesh) });
//	}
	
	void Renderer::submit(PointLight light) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		pointLights.push_back(light);
	}
	
	void Renderer::submit(SpotLight light) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		light.innerCutoff = std::cos(light.innerCutoff);
		light.outerCutoff = std::cos(light.outerCutoff);
		spotLights.push_back(light);
	}
	
	void Renderer::submit(DirectionalLight light) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		dirLights.push_back(light);
		
		if (!light.castsShadows) {
			return;
		}
		
		++numShadowCasters;
		numCascades.push_back(light.numCascades);
		
		float distance = light.shadowDistance;
		for (int i = 0; i < light.numCascades; ++i) {
			auto const lightSpaceTransform = directionalLightSpaceTransform(camera,
																			distance,
																			light.shadowDistanceZ,
																			light.direction);
			lightSpaceTransforms.push_back(mtl::transpose(lightSpaceTransform));
			distance *= light.cascadeDistributionExponent;
		}
	}
	
	void Renderer::submitShadowCascadeViz(DirectionalLight light) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		
		if (!light.castsShadows) {
			return;
		}
		
		shadowCascadeVizCount = light.numCascades;
		
		float distance = light.shadowDistance;
		
		for (int i = 0; i < light.numCascades; ++i) {
			auto const lightSpaceTransform = directionalLightSpaceTransform(camera,
																			distance,
																			light.shadowDistanceZ,
																			light.direction);
			shadowCascadeVizTransforms[i] = mtl::transpose(lightSpaceTransform);
			distance *= light.cascadeDistributionExponent;
		}
	}
	
	void Renderer::submit(SkyLight light) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		skyLights.push_back(light);
	}
	
	static auto const objectOrder = [](auto&& a, auto&& b) {
		if (a.material == b.material) {
			return a.mesh < b.mesh;
		}
		return a.material < b.material;
	};
	
	void Renderer::draw(Framebuffer& framebuffer, CommandQueue& commandQueue) {
		drawPrivate(framebuffer, commandQueue, &Renderer::mainPass);
	}
	
	void Renderer::drawWireframe(Framebuffer& framebuffer, CommandQueue& commandQueue) {
		drawPrivate(framebuffer, commandQueue, &Renderer::wireframePass);
	}
	
	void Renderer::drawPrivate(Framebuffer& framebuffer,
							   CommandQueue& commandQueue,
							   void(Renderer::*renderPass)(Framebuffer&, CommandQueue&))
	{
		std::sort(objects.begin(), objects.end(), objectOrder);
		uploadEntityData();
		SceneRenderData sceneRenderData;
		
		// Scene Data
		sceneRenderData.camera = mtl::transpose(camera.viewProjection());
		sceneRenderData.cameraPosition = camera.position();
		sceneRenderData.screenResolution = framebuffer.size();
		
		// Point Lights
		if (pointLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Point Lights");
			pointLights.resize(32);
		}
		sceneRenderData.numPointLights = pointLights.size();
		std::copy(pointLights.begin(), pointLights.end(), sceneRenderData.pointLights);
		
		// Spotlights
		if (spotLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Spot Lights");
			spotLights.resize(32);
		}
		sceneRenderData.numSpotLights = spotLights.size();
		std::copy(spotLights.begin(), spotLights.end(), sceneRenderData.spotLights);
		
		// Directional Lights
		if (dirLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Directional Lights");
			dirLights.resize(32);
		}
		sceneRenderData.numDirLights = dirLights.size();
		std::copy(dirLights.begin(), dirLights.end(), sceneRenderData.dirLights);
		
		// Skylights
		if (skyLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Sky Lights");
			skyLights.resize(32);
		}
		sceneRenderData.numSkyLights = skyLights.size();
		std::copy(skyLights.begin(), skyLights.end(), sceneRenderData.skyLights);
		
		mDevice->fillManagedBuffer(sceneDataBuffer, &sceneRenderData, sizeof sceneRenderData);
		
		shadowMapPass(commandQueue);
		
		(this->*renderPass)(framebuffer, commandQueue);
		
		postprocess(framebuffer, commandQueue);
	}
	
	void Renderer::drawDebugInfo(EditorFramebuffer& framebuffer,
								 CommandQueue& commandQueue,
								 DebugDrawOptions options)
	{
		// Debug Draw Data
		DebugDrawData debugDrawData;
		debugDrawData.selectionLineWidth = 3;
		debugDrawData.visualizeShadowCascades = options.visualizeShadowCascades;
		debugDrawData.shadowCascadeVizCount = shadowCascadeVizCount;
		std::copy(shadowCascadeVizTransforms.begin(),
				  shadowCascadeVizTransforms.end(),
				  debugDrawData.shadowCascadeVizTransforms);
		
		mDevice->fillManagedBuffer(debugDrawDataBuffer, &debugDrawData, sizeof debugDrawData);
		
		editorPass(framebuffer, commandQueue);
		
		editorPP(framebuffer, commandQueue, options);
	}
	
	void Renderer::drawSelection(EditorFramebuffer& framebuffer, CommandQueue& commandQueue) {
		drawPrivate(framebuffer, commandQueue, &Renderer::selectionPass);
		selectionPass(framebuffer, commandQueue);
	}
	
	void Renderer::postprocess(Framebuffer& framebuffer, CommandQueue& commandQueue) {
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			ColorAttachment caDesc{};
			caDesc.texture = framebuffer.finalImage;
			
			desc.colorAttachments.push_back(caDesc);
			
			ctx->begin(desc);
		}
		ctx->setPipeline(postprocessPipeline);
		
		ctx->setVertexBuffer(quadVB, 1);
		
		ctx->setFragmentBuffer(sceneDataBuffer, 0);
		ctx->setFragmentTexture(framebuffer.color, 0);
		ctx->setFragmentSampler(postprocessSampler, 0);
		
		DrawDescription desc{};
		desc.indexBuffer = quadIB;
		desc.indexCount = 6;
		
		ctx->draw(desc);
		
		
		ctx->end();
		ctx->commit();
	}
	
	void Renderer::mainPass(Framebuffer& framebuffer, CommandQueue& commandQueue) {
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			ColorAttachment caDesc{};
			caDesc.texture = framebuffer.color;
			caDesc.clearColor = { 1, 0, 1, 1 };
			caDesc.loadAction = LoadAction::clear;
			desc.colorAttachments.push_back(caDesc);
			
			DepthAttachment dDesc{};
			dDesc.texture = framebuffer.depth;
			
			desc.depthAttachment = dDesc;
			ctx->begin(desc);
		}
		
		// Vertex buffers
		ctx->setVertexBuffer(sceneDataBuffer, 0);
		
		// Fragment buffers
		ctx->setFragmentBuffer(sceneDataBuffer, 0);
		ctx->setFragmentBuffer(shadowDataBuffer, 1);
		ctx->setFragmentBuffer(shadowDataBuffer, 2, sizeof(ShadowRenderData) /* offset */);
		ctx->setFragmentTexture(shadowMapArray, 0);
		ctx->setFragmentSampler(shadowMapSampler, 0);
		
		Material* currentMaterial = nullptr;
		for (auto&& [index, object]: utl::enumerate(objects)) {
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				ctx->setPipeline(currentMaterial->mainPass);
				ctx->setTriangleCullMode(currentMaterial->cullMode);
				ctx->setDepthStencil(depthStencil);
			}
			
			ctx->setVertexBuffer(object.mesh->vertexBuffer, 1);
			ctx->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			
			DrawDescription desc{};
			desc.indexCount = object.mesh->indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = object.mesh->indexBuffer;
			ctx->draw(desc);
		}
		ctx->end();
		
		ctx->commit();
	}
	
	void Renderer::wireframePass(Framebuffer& framebuffer, CommandQueue& commandQueue) {
		
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			ColorAttachment caDesc{};
			caDesc.texture = framebuffer.color;
			caDesc.clearColor = { 1, 0, 1, 1 };
			caDesc.loadAction = LoadAction::clear;
			desc.colorAttachments.push_back(caDesc);
			
			DepthAttachment dDesc{};
			dDesc.texture = framebuffer.depth;
			
			desc.depthAttachment = dDesc;
			ctx->begin(desc);
		}
		
		
		// Vertex buffers
		ctx->setVertexBuffer(sceneDataBuffer, 0);
		
		ctx->setTriangleFillMode(TriangleFillMode::lines);
		
		Material* currentMaterial = nullptr;
		for (std::size_t index = 0;
			 auto&& object: objects)
		{
			utl_defer { ++index; };
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				ctx->setPipeline(wireframePassPipeline);
				ctx->setTriangleCullMode(TriangleCullMode::none);
			}
			
			ctx->setVertexBuffer(object.mesh->vertexBuffer, 1);
			ctx->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			
			DrawDescription desc{};
			desc.indexCount = object.mesh->indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = object.mesh->indexBuffer;
			ctx->draw(desc);
		}
		ctx->end();
		ctx->commit();
	}
	
	void Renderer::editorPass(EditorFramebuffer& framebuffer,
							  CommandQueue& commandQueue)
	{
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			ColorAttachment caDesc{};
			caDesc.texture = framebuffer.entityID;
			caDesc.clearColor = 1; // null entity
			caDesc.loadAction = LoadAction::clear;
			
			desc.colorAttachments.push_back(caDesc);
			
			caDesc.texture = framebuffer.shadowCascade;
			caDesc.clearColor = 0;
			caDesc.loadAction = LoadAction::clear;
			
			desc.colorAttachments.push_back(caDesc);
			
			DepthAttachment dDesc{};
			dDesc.texture = framebuffer.editorDepth;
			desc.depthAttachment = dDesc;
			ctx->begin(desc);
		}
		
		

		ctx->setVertexBuffer(sceneDataBuffer, 0);

		ctx->setFragmentBuffer(sceneDataBuffer, 0);
		ctx->setFragmentBuffer(debugDrawDataBuffer, 1);
		ctx->setFragmentBuffer(shadowDataBuffer, 2);
		ctx->setFragmentBuffer(shadowDataBuffer, 3, sizeof(ShadowRenderData) /* offset */);
		ctx->setFragmentTexture(shadowMapArray, 0);
		ctx->setFragmentSampler(shadowMapSampler, 0);

		Material* currentMaterial = nullptr;
		
		for (auto&& [index, object]: utl::enumerate(objects)) {
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				ctx->setPipeline(editorPassPipeline);
				ctx->setTriangleCullMode(currentMaterial->cullMode);
				ctx->setDepthStencil(depthStencil);
			}
			ctx->setVertexBuffer(object.mesh->vertexBuffer, 1);
			ctx->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			
			DrawDescription desc{};
			desc.indexCount = object.mesh->indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = object.mesh->indexBuffer;
			ctx->draw(desc);
			
			ctx->draw(desc);
		}

		ctx->end();
		ctx->commit();
	}
	
	void Renderer::shadowMapPass(CommandQueue& commandQueue) {
		uploadShadowData();
		
		if (numShadowCasters == 0) {
			return;
		}
	
		std::size_t const numShadowMaps = std::accumulate(numCascades.begin(),
														  numCascades.end(), 0);
		
		if (numShadowMaps > shadowMapArrayLength || needsNewShadowMaps) {
			shadowMapArray = createShadowMaps(numShadowMaps);
		}
		
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			DepthAttachment dDesc{};
			dDesc.texture = shadowMapArray;
			desc.depthAttachment = dDesc;
			desc.renderTargetArrayLength = numShadowMaps;
			desc.renderTargetSize = shadowMapResolution;
			
			ctx->begin(desc);
		}
		
		ctx->setVertexBuffer(sceneDataBuffer, 0);
		ctx->setFragmentBuffer(sceneDataBuffer, 0);
		
		ctx->setPipeline(shadowPipeline);
		ctx->setTriangleCullMode(shadowCullMode); /// TODO: temporary
		ctx->setDepthStencil(depthStencil);
		
		for (auto&& [index, object]: utl::enumerate(objects)) {
			ctx->setVertexBuffer(object.mesh->vertexBuffer, 1);
			ctx->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			ctx->setVertexBuffer(shadowDataBuffer, 3, /* offset = */ sizeof(ShadowRenderData) /* <- this is the header of the buffer */);
			
			DrawDescription desc{};
			desc.indexCount = object.mesh->indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = object.mesh->indexBuffer;
			desc.instanceCount = numShadowMaps;
			ctx->draw(desc);
		}
		
		ctx->end();
		ctx->commit();
	}
	
	void Renderer::selectionPass(Framebuffer& fb, CommandQueue& commandQueue) {
		EditorFramebuffer& framebuffer = static_cast<EditorFramebuffer&>(fb);
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			ColorAttachment caDesc;
			caDesc.texture = framebuffer.selected;
			caDesc.loadAction = LoadAction::clear;
			caDesc.clearColor = 0;
			
			desc.colorAttachments.push_back(caDesc);
		
			
			ctx->begin(desc);
		}
		ctx->setVertexBuffer(sceneDataBuffer, 0);
		
		Material* currentMaterial = nullptr;
		for (auto&& [index, object]: utl::enumerate(objects)) {
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				ctx->setPipeline(currentMaterial->outlinePass);
			}
			ctx->setVertexBuffer(object.mesh->vertexBuffer, 1);
			ctx->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			
			DrawDescription desc{};
			desc.indexCount = object.mesh->indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = object.mesh->indexBuffer;
			ctx->draw(desc);
			
			ctx->draw(desc);
		}
	
		ctx->end();
		ctx->commit();
	}
	
	void Renderer::editorPP(EditorFramebuffer& framebuffer,
							CommandQueue& commandQueue,
							DebugDrawOptions options)
	{
		std::unique_ptr ctx = commandQueue.createRenderContext();
		
		{
			RenderPassDescription desc{};
			ColorAttachment caDesc;
			caDesc.texture = framebuffer.finalImageEditor;
			
			desc.colorAttachments.push_back(caDesc);
			
			ctx->begin(desc);
		}
		
		ctx->setPipeline(editorPPPipeline);
		ctx->setVertexBuffer(quadVB, 1);
		
		ctx->setFragmentBuffer(sceneDataBuffer, 0);
		ctx->setFragmentBuffer(debugDrawDataBuffer, 1);
		ctx->setFragmentTexture(framebuffer.finalImage, 0);
		ctx->setFragmentTexture(framebuffer.selected, 1);
		ctx->setFragmentTexture(framebuffer.shadowCascade, 2);
		ctx->setFragmentSampler(postprocessSampler, 0);
		
		DrawDescription desc{};
		desc.indexCount = 6;
		desc.indexType = IndexType::uint32;
		desc.indexBuffer = quadIB;
		ctx->draw(desc);
		
		ctx->end();
		ctx->commit();
	}
	
	static void uploadEntityDataEx(auto&& objects, auto&& entityDataBuffer, HardwareDevice& device) {
		std::size_t const size = objects.size() * sizeof(EntityRenderData);
		if (size == 0) {
			return;
		}
		if (entityDataBuffer.size() < size) {
			BufferDescription desc;
			desc.size = size;
			desc.storageMode = StorageMode::managed;
			entityDataBuffer = device.createBuffer(desc);
		}
		device.fillManagedBuffer(entityDataBuffer, objects.data().entity, size);
	}
	
	void Renderer::uploadEntityData() {
		uploadEntityDataEx(objects, entityDataBuffer, device());
	}
	
	void Renderer::uploadSceneData() {
		
	}
	
	void Renderer::uploadDebugDrawData() {
		
	}
	
	void Renderer::setShadowMapResolution(mtl::uint2 r) {
		shadowMapResolution = r;
		needsNewShadowMaps = true;
	}
	
	void Renderer::createShadowPipeline() {
		RenderPipelineDescription desc;
		desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
		desc.vertexFunction = device().createFunction("shadowVertexShader");
		
		desc.rasterSampleCount = 1;
		desc.inputPrimitiveTopology = PrimitiveTopologyClass::triangle;
		
		shadowPipeline = device().createRenderPipeline(desc);
	}
	
	void Renderer::createShadowMapSampler() {
		SamplerDescription desc;
		shadowMapSampler = device().createSampler(desc);
	}
	
	void Renderer::uploadShadowData() {
		ShadowRenderData header;
		header.numShadowCasters = numShadowCasters;
		bloomAssert(numShadowCasters == numCascades.size());
		std::copy(numCascades.begin(), numCascades.end(), std::begin(header.numCascades));
		
		std::size_t const size = sizeof(ShadowRenderData) + std::max(lightSpaceTransforms.size(), std::size_t{ 1 }) * sizeof(mtl::float4x4);
		
		if (shadowDataBuffer.size() < size) {
			BufferDescription desc;
			desc.size = size;
			desc.storageMode = StorageMode::managed;
			shadowDataBuffer = device().createBuffer(desc);
		}
		device().fillManagedBuffer(shadowDataBuffer,
								   &header,
								   sizeof header);
		device().fillManagedBuffer(shadowDataBuffer,
								   lightSpaceTransforms.data(),
								   lightSpaceTransforms.size() * sizeof(mtl::float4x4),
								   sizeof header /* offset */);
	}
	
	TextureHandle Renderer::createShadowMaps(int totalShadowMaps) {
		shadowMapArrayLength = totalShadowMaps;
		needsNewShadowMaps = false;
		
		TextureDescription desc;
		desc.type = TextureType::texture2DArray;
		desc.size = usize3(shadowMapResolution, 1);
		desc.arrayLength = totalShadowMaps;
		desc.pixelFormat = PixelFormat::Depth32Float;
		desc.usage = TextureUsage::renderTarget | TextureUsage::shaderRead;
		desc.storageMode = StorageMode::GPUOnly;
		
		return device().createTexture(desc);
	}
	
	void Renderer::createEditorPassPipeline() {
		RenderPipelineDescription desc;
		
		ColorAttachmentDescription caDesc;
		
		caDesc.pixelFormat = PixelFormat::R32Uint;
		desc.colorAttachments.push_back(caDesc);
		
		caDesc.pixelFormat = PixelFormat::RGBA8Unorm;
		desc.colorAttachments.push_back(caDesc);
		
		desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
		
		desc.vertexFunction = device().createFunction("editorPassVS");
		desc.fragmentFunction = device().createFunction("editorPassFS");
		
		editorPassPipeline = device().createRenderPipeline(desc);
	}
	
	void Renderer::createWireframePassPipeline() {
		RenderPipelineDescription desc;
		
		ColorAttachmentDescription caDesc;
		
		caDesc.pixelFormat = PixelFormat::R32Float;
		desc.colorAttachments.push_back(caDesc);
		
		desc.vertexFunction = device().createFunction("editorPassVS");
		desc.fragmentFunction = device().createFunction("wireframePassFS");
		
		wireframePassPipeline = device().createRenderPipeline(desc);
	}
	
	void Renderer::createPostprocessQuad() {
		mtl::float2 const vertices[] = {
			{ -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
		};
		BufferDescription desc;
		desc.size = sizeof vertices;
		desc.data = vertices;
		desc.storageMode = StorageMode::shared;
		quadVB = device().createBuffer(desc);
		
		std::uint32_t const indices[] = {
			0, 1, 2, 1, 3, 2
		};
		
		desc.size = sizeof indices;
		desc.data = indices;
		desc.storageMode = StorageMode::shared;
		quadIB = device().createBuffer(desc);
	}
	
	void Renderer::createPostprocessPipelines() {
		// ---------------
		RenderPipelineDescription desc;
		
		ColorAttachmentDescription caDesc;
		
		caDesc.pixelFormat = PixelFormat::RGBA8Unorm;
		desc.colorAttachments.push_back(caDesc);
		
		desc.vertexFunction = device().createFunction("postprocessVS");
		desc.fragmentFunction = device().createFunction("postprocess");
		
		postprocessPipeline = device().createRenderPipeline(desc);
		
		
		// ---------------
		desc.vertexFunction = device().createFunction("postprocessVS");
		desc.fragmentFunction = device().createFunction("editorPP");
		
		editorPPPipeline = device().createRenderPipeline(desc);
	}
	
	void Renderer::createPostprocessSampler() {
		SamplerDescription desc;
		
		postprocessSampler = device().createSampler(desc);
	}

	
	
}
