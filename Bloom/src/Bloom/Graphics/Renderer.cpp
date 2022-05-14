#define UTL_DEFER_MACROS

#include "Renderer.hpp"

#include <algorithm>
#include <numeric>
#include <array>
#include <utl/scope_guard.hpp>

#include "Bloom/Core/Debug.hpp"
#include "SceneRenderData.hpp"

#include "RenderContext.hpp"
#include "FrameBuffer.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "StaticRenderMesh.hpp"
#include "RenderContext.hpp"

#include "Bloom/Platform/Metal/MetalRenderContext.hpp"
#include <Metal/Metal.hpp>

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
	
	std::string_view toString(DebugDrawOptions::Mode mode) {
		return std::array{
			"Lit",
			"Wireframe"
		}[(std::size_t)mode];
	}
	
	std::ostream& operator<<(std::ostream& stream, DebugDrawOptions::Mode mode) {
		return stream << toString(mode);
	}
	
	void Renderer::init(RenderContext* renderContext) {
		this->renderContext = renderContext;
		
		sceneDataBuffer = renderContext->createUniformBuffer(nullptr, sizeof(SceneRenderData));
		debugDrawDataBuffer = renderContext->createUniformBuffer(nullptr, sizeof(DebugDrawData));
		depthStencil = renderContext->createDepthStencilState(CompareFunction::lessEqual);
		
		createShadowPipeline();
		createShadowMapSampler();
		
		createEditorPassPipeline();
		createWireframePassPipeline();
		
		createPostprocessQuad();
		createPostprocessPipelines();
		createPostprocessSampler();
	}
	
	void Renderer::beginScene(Camera const& camera, DebugDrawOptions options) {
		bloomExpect(!buildingScene, "Already called beginScene?");
		buildingScene = true;
		this->options = options;
		
		this->camera = camera;
		objects.clear();
		selectedObjects.clear();
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
	
	void Renderer::submitSelected(Reference<StaticRenderMesh> mesh, Reference<Material> material, EntityRenderData entityData) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		entityData.transform = mtl::transpose(entityData.transform);
		selectedObjects.push_back({ entityData, std::move(material), std::move(mesh) });
	}
	
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
	
	void Renderer::draw(FrameBuffer* frameBuffer) {
		std::sort(objects.begin(), objects.end(), objectOrder);
		uploadEntityData();
		SceneRenderData sceneRenderData;
		
		// Scene Data
		sceneRenderData.camera = mtl::transpose(camera.viewProjection());
		sceneRenderData.cameraPosition = camera.position();
		sceneRenderData.screenResolution = frameBuffer->size();
		
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
		
		renderContext->fillBuffer(sceneDataBuffer, &sceneRenderData, sizeof sceneRenderData);
		
		
		
		auto shadowHandle = shadowMapPass();
		shadowHandle.wait();
		
		auto mainHandle = options.mode == DebugDrawOptions::Mode::wireframe ?
			wireframePass(frameBuffer) : mainPass(frameBuffer);
		
//		mainHandle.wait();
//		editorHandle.wait();
//		outlineHandle.wait();
		
		postprocess(frameBuffer);
	}
	
	
	void Renderer::drawDebugInfo(EditorFrameBuffer* frameBuffer) {
		// Debug Draw Data
		DebugDrawData debugDrawData;
		debugDrawData.selectionLineWidth = 3;
		debugDrawData.visualizeShadowCascades = options.visualizeShadowCascades;
		debugDrawData.shadowCascadeVizCount = shadowCascadeVizCount;
		std::copy(shadowCascadeVizTransforms.begin(),
				  shadowCascadeVizTransforms.end(),
				  debugDrawData.shadowCascadeVizTransforms);
		
		renderContext->fillBuffer(debugDrawDataBuffer, &debugDrawData, sizeof debugDrawData);
		
		auto editorHandle = editorPass(frameBuffer, options.mode);
		auto outlineHandle = selectionPass(frameBuffer);
		editorPP(frameBuffer, options.visualizeShadowCascades, options.lightVizEntityID);
	}
	
	void Renderer::postprocess(FrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->finalImage);
		
		renderContext->beginRenderPass();
		
		renderContext->setPipelineState(postprocessPipeline);
		renderContext->setVertexBuffer(quadVB, 1);
		
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentTexture(frameBuffer->color, 0);
		renderContext->setFragmentSampler(postprocessSampler, 0);
		
		renderContext->drawIndexed(quadIB, bloom::IndexType::uint32);
		
		renderContext->commit().wait();
	}
	
	RenderPassHandle Renderer::mainPass(FrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->color);
		renderContext->setClearColor(0, { 0,0,0,0 });
		renderContext->setRenderTargetDepth(frameBuffer->depth);
		
		renderContext->beginRenderPass();
		
		// Vertex buffers
		renderContext->setVertexBuffer(sceneDataBuffer, 0);
		
		// Fragment buffers
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentBuffer(shadowDataBuffer, 1);
		renderContext->setFragmentBuffer(shadowDataBuffer, 2, sizeof(ShadowRenderData) /* offset */);
		renderContext->setFragmentTexture(shadowMapArray, 0);
		renderContext->setFragmentSampler(shadowMapSampler, 0);
		
		Material* currentMaterial = nullptr;
		for (std::size_t index = 0;
			 auto&& object: objects)
		{
			utl_defer { ++index; };
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				renderContext->setPipelineState(currentMaterial->mainPass);
				renderContext->setTriangleCullMode(currentMaterial->cullMode);
				renderContext->setDepthStencilState(depthStencil);
			}
			
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			renderContext->drawIndexed(object.mesh->indexBuffer, bloom::IndexType::uint32);
		}
		
		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::wireframePass(FrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->color);
		renderContext->setClearColor(0, { 0,0,0,0 });
		
		renderContext->beginRenderPass();
		
		// Vertex buffers
		renderContext->setVertexBuffer(sceneDataBuffer, 0);
		
		renderContext->setTriangleFillMode(TriangleFillMode::lines);
		
		Material* currentMaterial = nullptr;
		for (std::size_t index = 0;
			 auto&& object: objects)
		{
			utl_defer { ++index; };
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				renderContext->setPipelineState(wireframePassPipeline);
				renderContext->setTriangleCullMode(TriangleCullMode::none);
			}
			
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			renderContext->drawIndexed(object.mesh->indexBuffer, bloom::IndexType::uint32);
		}
		
		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::editorPass(EditorFrameBuffer* frameBuffer, DebugDrawOptions::Mode mode) {
		renderContext->setRenderTargetColor(0, frameBuffer->entityID);
		renderContext->setClearColor(0, 0xFFffFFff /* null entity */);

		renderContext->setRenderTargetColor(1, frameBuffer->shadowCascade);
		renderContext->setClearColor(1, 0);

		renderContext->setRenderTargetDepth(frameBuffer->editorDepth);

		Material* currentMaterial = nullptr;

		renderContext->beginRenderPass();

		renderContext->setVertexBuffer(sceneDataBuffer, 0);

		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentBuffer(debugDrawDataBuffer, 1);
		renderContext->setFragmentBuffer(shadowDataBuffer, 2);
		renderContext->setFragmentBuffer(shadowDataBuffer, 3, sizeof(ShadowRenderData) /* offset */);
		renderContext->setFragmentTexture(shadowMapArray, 0);
		renderContext->setFragmentSampler(shadowMapSampler, 0);

		for (std::size_t index = 0;
			 auto object: objects)
		{
			utl_defer { ++index; };
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				renderContext->setPipelineState(editorPassPipeline);
				renderContext->setTriangleCullMode(currentMaterial->cullMode);
				renderContext->setDepthStencilState(depthStencil);
			}
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			renderContext->drawIndexed(object.mesh->indexBuffer, bloom::IndexType::uint32);
		}

		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::shadowMapPass() {
		uploadShadowData();
		
		if (numShadowCasters == 0) {
			return {};
		}
	
		std::size_t const numShadowMaps = std::accumulate(numCascades.begin(),
														  numCascades.end(), 0);
		
		if (numShadowMaps > shadowMapArrayLength || needsNewShadowMaps) {
			shadowMapArray = createShadowMaps(numShadowMaps);
		}
		
		renderContext->setRenderTargetDepth(shadowMapArray);
		renderContext->setRenderTargetArrayLength(numShadowMaps);
		renderContext->setRenderTargetSize(shadowMapResolution);
		renderContext->setDefaultRasterSampleCount(1);
		
		renderContext->beginRenderPass();
		
		renderContext->setVertexBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		
		renderContext->setPipelineState(shadowPipeline);
		renderContext->setTriangleCullMode(shadowCullMode); /// TODO: temporary
		renderContext->setDepthStencilState(depthStencil);
		
		for (std::size_t objectIndex = 0;
			 auto object: objects)
		{
			utl_defer { ++objectIndex; };
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(entityDataBuffer, 2, objectIndex * sizeof(EntityRenderData));
			renderContext->setVertexBuffer(shadowDataBuffer, 3, /* offset = */ sizeof(ShadowRenderData) /* <- this is the header of the buffer */);
			renderContext->drawIndexedInstances(object.mesh->indexBuffer,
												bloom::IndexType::uint32,
												numShadowMaps);
		}
		
		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::selectionPass(EditorFrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->selected);
		renderContext->setClearColor(0, 0.0f);
		
		Material* currentMaterial = nullptr;
		
		renderContext->beginRenderPass();
		
		renderContext->setVertexBuffer(sceneDataBuffer, 0);
	
		for (std::size_t index = 0;
			 auto object: selectedObjects)
		{
			utl_defer { ++index; };
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				renderContext->setPipelineState(currentMaterial->outlinePass);
			}
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(selectedEntityDataBuffer, 2, index * sizeof(EntityRenderData));
			renderContext->drawIndexed(object.mesh->indexBuffer, bloom::IndexType::uint32);
		}
		
		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::editorPP(EditorFrameBuffer* frameBuffer, bool vizShadowCascades, uint32_t lightEntity) {
		renderContext->setRenderTargetColor(0, frameBuffer->finalImageEditor);
		
		
		renderContext->beginRenderPass();
		
		renderContext->setPipelineState(editorPPPipeline);
		renderContext->setVertexBuffer(quadVB, 1);
		
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentBuffer(debugDrawDataBuffer, 1);
		renderContext->setFragmentTexture(frameBuffer->finalImage, 0);
		renderContext->setFragmentTexture(frameBuffer->selected, 1);
		renderContext->setFragmentTexture(frameBuffer->shadowCascade, 2);
		renderContext->setFragmentSampler(postprocessSampler, 0);
		
		renderContext->drawIndexed(quadIB, bloom::IndexType::uint32);
		
		return renderContext->commit();
	}
	
	static void uploadEntityDataEx(auto&& objects, auto&& entityDataBuffer, auto renderContext) {
		std::size_t const size = objects.size() * sizeof(EntityRenderData);
		if (size == 0) {
			return;
		}
		if (entityDataBuffer.size() < size) {
			entityDataBuffer = renderContext->createUniformBuffer(nullptr, size);
		}
		renderContext->fillBuffer(entityDataBuffer, objects.data().entity, size);
	}
	
	void Renderer::uploadEntityData() {
		uploadEntityDataEx(objects, entityDataBuffer, renderContext);
		uploadEntityDataEx(selectedObjects, selectedEntityDataBuffer, renderContext);
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
		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		ARCPointer lib = device->newDefaultLibrary();
		
		ARCPointer psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
		
		psDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
		
		psDesc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
		psDesc->setSampleCount(1);
		
		psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("shadowVertexShader")));
//		psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("shadowFragmentShader"))); // no fragment function needed

		NS::Error* errors = nullptr;
		auto* ps = device->newRenderPipelineState(psDesc.get(), &errors);
		assert(ps);
		assert(!errors);
		
		shadowPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
	}
	
	void Renderer::createShadowMapSampler() {
		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		
		ARCPointer desc = MTL::SamplerDescriptor::alloc()->init();
		desc->setMinFilter(MTL::SamplerMinMagFilterNearest);
		desc->setMagFilter(MTL::SamplerMinMagFilterNearest);
		desc->setMipFilter(MTL::SamplerMipFilterNearest);
		desc->setMaxAnisotropy(1);
		desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		desc->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		desc->setNormalizedCoordinates(true);

		auto* sampler = device->newSamplerState(desc.get());
		shadowMapSampler = SamplerHandle(sampler, MTLDeleter);
	}
	
	void Renderer::uploadShadowData() {
		ShadowRenderData header;
		header.numShadowCasters = numShadowCasters;
		bloomAssert(numShadowCasters == numCascades.size());
		std::copy(numCascades.begin(), numCascades.end(), std::begin(header.numCascades));
		
		std::size_t const size = sizeof(ShadowRenderData) + std::max(lightSpaceTransforms.size(), std::size_t{ 1 }) * sizeof(mtl::float4x4);
		
		if (shadowDataBuffer.size() < size) {
			shadowDataBuffer = renderContext->createUniformBuffer(nullptr, size);
		}
		renderContext->fillBuffer(shadowDataBuffer,
								  &header,
								  sizeof header);
		renderContext->fillBuffer(shadowDataBuffer,
								  lightSpaceTransforms.data(),
								  lightSpaceTransforms.size() * sizeof(mtl::float4x4),
								  sizeof header /* offset */);
	}
	
	TextureHandle Renderer::createShadowMaps(int totalShadowMaps) {
		shadowMapArrayLength = totalShadowMaps;
		needsNewShadowMaps = false;
		return renderContext->createArrayTexture(shadowMapResolution,
												 totalShadowMaps,
												 PixelFormat::Depth32Float,
												 TextureUsage::renderTarget | TextureUsage::shaderRead,
												 StorageMode::Private);
	}
	
	void Renderer::createEditorPassPipeline() {
		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		auto* lib = device->newDefaultLibrary();
		
		auto* psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
		psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatR32Uint);
		psDesc->colorAttachments()->object(1)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
		psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("editorPassVS")));
		psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("editorPassFS")));
		psDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

		NS::Error* errors;
		auto* ps = device->newRenderPipelineState(psDesc, &errors);
		assert(ps);
		assert(!errors);
		editorPassPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
		
		psDesc->release();
	}
	
	void Renderer::createWireframePassPipeline() {
		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		auto* lib = device->newDefaultLibrary();
		
		auto* psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
		psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA32Float);
		psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("editorPassVS")));
		psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("wireframePassFS")));

		NS::Error* errors;
		auto* ps = device->newRenderPipelineState(psDesc, &errors);
		assert(ps);
		assert(!errors);
		wireframePassPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
		
		psDesc->release();
	}
	
	void Renderer::createPostprocessQuad() {
		mtl::float2 const vertices[] = {
			{ -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
		};
		quadVB = renderContext->createVertexBuffer(vertices, sizeof vertices);
		
		std::uint32_t const indices[] = {
			0, 1, 2, 1, 3, 2
		};
		quadIB = renderContext->createIndexBuffer(indices);
	}
	
	void Renderer::createPostprocessPipelines() {
		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		ARCPointer lib = device->newDefaultLibrary();
		
		/* postprocessPipeline */ {
			ARCPointer psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
			psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("postprocessVS")));
			psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("postprocess")));

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc.get(), &errors);
			assert(ps);
			assert(!errors);
			postprocessPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
		}
		
		/* editorPPPipeline */ {
			ARCPointer psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
			psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("postprocessVS")));
			psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("editorPP")));

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc.get(), &errors);
			assert(ps);
			assert(!errors);
			editorPPPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
		}
	}
	
	void Renderer::createPostprocessSampler() {
		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		
		ARCPointer desc = MTL::SamplerDescriptor::alloc()->init();
		desc->setMinFilter(MTL::SamplerMinMagFilterLinear);
		desc->setMagFilter(MTL::SamplerMinMagFilterLinear);
		desc->setMipFilter(MTL::SamplerMipFilterLinear);
		desc->setMaxAnisotropy(1);
		desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		desc->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		desc->setNormalizedCoordinates(true);

		auto* sampler = device->newSamplerState(desc.get());
		postprocessSampler = SamplerHandle(sampler, MTLDeleter);
	}

	
	
}
