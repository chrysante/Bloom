#include "ForwardRenderer.hpp"

#include "Bloom/GPU/HardwareDevice.hpp"
#include "Bloom/Graphics/Material/Material.hpp"
#include "Bloom/Graphics/StaticMesh.hpp"

#include <numeric>
#include <utl/utility.hpp>

using namespace mtl::short_types;

namespace bloom {

	/// MARK: Framebuffer Creation
	///
	///
	std::unique_ptr<Framebuffer> ForwardRenderer::createFramebuffer(mtl::int2 size) const {
		auto framebuffer = std::make_unique<ForwardRendererFramebuffer>();
		populateFramebuffer(device(), *framebuffer, size);
		
		return std::move(framebuffer);
	}

	std::unique_ptr<Framebuffer> ForwardRenderer::createDebugFramebuffer(mtl::int2 size) const {
		auto framebuffer = std::make_unique<ForwardRendererDebugFramebuffer>();
		
		populateFramebuffer(device(), *framebuffer, size);;
		
		TextureDescription desc;
		desc.size = { size, 1 };
		desc.type = TextureType::texture2D;
		desc.mipmapLevelCount = 1;
		desc.storageMode = StorageMode::GPUOnly;
		desc.usage = TextureUsage::shaderRead | TextureUsage::renderTarget;
		desc.pixelFormat = PixelFormat::RGBA8Unorm;
		framebuffer->shadowCascade = device().createTexture(desc);
		
		return std::move(framebuffer);
	}
	
	void ForwardRenderer::populateFramebuffer(HardwareDevice& device,
											  ForwardRendererFramebuffer& framebuffer,
											  mtl::usize2 size) const
	{
		framebuffer.size = size;
		TextureDescription desc;
		desc.size = { size, 1 };
		desc.type = TextureType::texture2D;
		desc.mipmapLevelCount = 1;
		desc.storageMode = StorageMode::GPUOnly;
		desc.usage = TextureUsage::shaderRead | TextureUsage::renderTarget;
		
		desc.pixelFormat = PixelFormat::Depth32Float;
		framebuffer.depth = device.createTexture(desc);
		
		desc.pixelFormat = PixelFormat::RGBA32Float;
		framebuffer.rawColor = device.createTexture(desc);
		
		desc.pixelFormat = PixelFormat::RGBA8Unorm;
		desc.usage = TextureUsage::shaderRead | TextureUsage::shaderWrite;
		framebuffer.postProcessed = device.createTexture(desc);
		
		// Bloom / Veil
		bloomRenderer.populateFramebuffer(device, framebuffer.bloom, size);
	}
	
	/// MARK: Initialization
	///
	///
	ForwardRenderer::ForwardRenderer(bloom::Reciever reciever): Renderer(std::move(reciever)) {
		listen([this](ReloadShadersCommand){
			device().reloadDefaultLibrary();
			createGPUState(device());
		});
	}
	
	static RenderPipelineHandle createShadowPipeline(HardwareDevice& device) {
		RenderPipelineDescription desc;
		desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
		desc.vertexFunction = device.createFunction("shadowVertexShader");
		
		desc.rasterSampleCount = 1;
		desc.inputPrimitiveTopology = PrimitiveTopologyClass::triangle;
		
		return device.createRenderPipeline(desc);
	}
	
	static ComputePipelineHandle createPostprocessPipeline(HardwareDevice& device) {
		// ---------------
		ComputePipelineDescription desc;
		
		desc.computeFunction = device.createFunction("postprocess");
		
		return device.createComputePipeline(desc);
	}
	
	void ForwardRenderer::init(HardwareDevice& device) {
		RendererSanitizer::init();
		
		mDevice = &device;
		
		createGPUState(device);
		bloomRenderer.init(device);
	}
	
	void ForwardRenderer::createGPUState(HardwareDevice& device) {
		/* Buffers */ {
			BufferDescription desc{};
			desc.storageMode = StorageMode::managed;
			
			desc.size = sizeof(SceneRenderData);
			renderObjects.sceneDataBuffer = device.createBuffer(desc);
			
			desc.size = sizeof(ForwardRendererParameters);
			renderObjects.rendererParameters = device.createBuffer(desc);
		}
		{
			DepthStencilDescription desc{};
			desc.depthWrite = true;
			desc.depthCompareFunction = CompareFunction::lessEqual;
			
			renderObjects.depthStencil = device.createDepthStencil(desc);
		}
		
		renderObjects.shadows.pipeline = createShadowPipeline(device);
		renderObjects.shadows.sampler = device.createSampler(SamplerDescription{});
		
		renderObjects.postprocessSampler = device.createSampler(SamplerDescription{});
		renderObjects.postprocessPipeline = createPostprocessPipeline(device);
	}
	
	/// MARK: Scene Construction
	///
	/// 
	void ForwardRenderer::beginScene(Camera const& camera) {
		RendererSanitizer::beginScene();
		scene.clear();
		scene.camera = camera;
	}
	
	static auto const objectOrder = [](auto&& a, auto&& b) {
		if (a.material == b.material) {
			return a.mesh < b.mesh;
		}
		return a.material < b.material;
	};
	
	void ForwardRenderer::endScene() {
		RendererSanitizer::endScene();
		
		std::sort(scene.objects.begin(), scene.objects.end(), objectOrder);
		
		sceneRenderData.camera = mtl::transpose(scene.camera.viewProjection());
		sceneRenderData.cameraPosition = scene.camera.position();
		
		// Point Lights
		if (scene.pointLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Point Lights");
			scene.pointLights.resize(32);
		}
		sceneRenderData.numPointLights = scene.pointLights.size();
		std::copy(scene.pointLights.begin(), scene.pointLights.end(),
				  sceneRenderData.pointLights);
		
		// Spotlights
		if (scene.spotLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Spot Lights");
			scene.spotLights.resize(32);
		}
		sceneRenderData.numSpotLights = scene.spotLights.size();
		std::copy(scene.spotLights.begin(), scene.spotLights.end(),
				  sceneRenderData.spotLights);
		
		// Directional Lights
		if (scene.dirLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Directional Lights");
			scene.dirLights.resize(32);
		}
		sceneRenderData.numDirLights = scene.dirLights.size();
		std::copy(scene.dirLights.begin(), scene.dirLights.end(),
				  sceneRenderData.dirLights);
		
		// Skylights
		if (scene.skyLights.size() > 32) {
			bloomLog(warning, "Can't render more than 32 Sky Lights");
			scene.skyLights.resize(32);
		}
		sceneRenderData.numSkyLights = scene.skyLights.size();
		std::copy(scene.skyLights.begin(), scene.skyLights.end(),
				  sceneRenderData.skyLights);
		
		/* upload object data */ do {
			std::size_t const size = scene.objects.size() * sizeof(float4x4);
			if (size == 0) {
				break;
			}
			if (renderObjects.transformBuffer.size() < size) {
				BufferDescription desc;
				desc.size = size;
				desc.storageMode = StorageMode::managed;
				renderObjects.transformBuffer = device().createBuffer(desc);
			}
			device().fillManagedBuffer(renderObjects.transformBuffer,
									   scene.objects.data().transform,
									   size);
		} while (0);
		
		/* upload shadow data */ {
			ShadowRenderData header;
			header.numShadowCasters = scene.shadows.numShadowCasters;
			bloomAssert(scene.shadows.numShadowCasters == scene.shadows.numCascades.size());
			std::copy(scene.shadows.numCascades.begin(),
					  scene.shadows.numCascades.end(),
					  std::begin(header.numCascades));
			
			std::size_t const size = sizeof(ShadowRenderData) + std::max(scene.shadows.lightSpaceTransforms.size(), std::size_t{ 1 }) * sizeof(float4x4);
			
			if (renderObjects.shadows.dataBuffer.size() < size) {
				BufferDescription desc;
				desc.size = size;
				desc.storageMode = StorageMode::managed;
				renderObjects.shadows.dataBuffer = device().createBuffer(desc);
			}
			device().fillManagedBuffer(renderObjects.shadows.dataBuffer,
									   &header,
									   sizeof header);
			device().fillManagedBuffer(renderObjects.shadows.dataBuffer,
									   scene.shadows.lightSpaceTransforms.data(),
									   scene.shadows.lightSpaceTransforms.size() * sizeof(float4x4),
									   sizeof header /* offset */);
		}
	}
	
	void ForwardRenderer::submit(Reference<StaticMeshRenderer> mesh, Reference<Material> material, mtl::float4x4 const& transform) {
		RendererSanitizer::submit();
		scene.objects.push_back({ mtl::transpose(transform), std::move(material), std::move(mesh) });
	}
	
	void ForwardRenderer::submit(PointLight const& light) {
		RendererSanitizer::submit();
		scene.pointLights.push_back(light);
	}
	
	void ForwardRenderer::submit(SpotLight const& l) {
		RendererSanitizer::submit();
		auto light = l;
		light.innerCutoff = std::cos(light.innerCutoff);
		light.outerCutoff = std::cos(light.outerCutoff);
		scene.spotLights.push_back(light);
	}
	
	static mtl::float4x4 directionalLightSpaceTransform(Camera const& camera,
														float dist,
														float zDist,
														mtl::float3 lightDirection)
	{
		mtl::float4x4 const lsProj = mtl::ortho<mtl::right_handed>(-dist, dist,
																   -dist, dist,
																   -zDist, zDist);

		mtl::float4x4 const lsView = mtl::look_at<mtl::right_handed>(camera.position(),
																	 camera.position() - lightDirection,
																	 { 0, 1, 0 });
		return lsProj * lsView;
	}
	
	void ForwardRenderer::submit(DirectionalLight const& light) {
		RendererSanitizer::submit();
		
		scene.dirLights.push_back(light);
		
		if (!light.castsShadows) {
			return;
		}
		
		++scene.shadows.numShadowCasters;
		scene.shadows.numCascades.push_back(light.numCascades);
		
		float distance = light.shadowDistance;
		for (int i = 0; i < light.numCascades; ++i) {
			auto const lightSpaceTransform = directionalLightSpaceTransform(scene.camera,
																			distance,
																			light.shadowDistanceZ,
																			light.direction);
			scene.shadows.lightSpaceTransforms.push_back(mtl::transpose(lightSpaceTransform));
			distance *= light.cascadeDistributionExponent;
		}
	}
	
	void ForwardRenderer::submit(SkyLight const& light) {
		RendererSanitizer::submit();
		scene.skyLights.push_back(light);
	}
	
	/// MARK: Draw
	void ForwardRenderer::draw(Framebuffer& fb, CommandQueue& commandQueue) {
		auto& framebuffer = utl::down_cast<ForwardRendererFramebuffer&>(fb);
		
		/* upload scene data buffer */ {
			// this step is delayed because in endScene() we don't know the size of the framebuffer yet
			sceneRenderData.screenResolution = framebuffer.size;
			mDevice->fillManagedBuffer(renderObjects.sceneDataBuffer, &sceneRenderData, sizeof sceneRenderData);
		}
		
		/* upload renderer shader parameters */ {
			ForwardRendererParameters params{};
			params.postprocess.bloom = bloomRenderer.makeShaderParameters();
			device().fillManagedBuffer(renderObjects.rendererParameters,
									   &params, sizeof params);
		}
		
		shadowMapPass(commandQueue);
		
		mainPass(framebuffer, commandQueue);
		
		bloomRenderer.render(commandQueue,
							 framebuffer.bloom,
							 framebuffer.rawColor,
							 renderObjects.rendererParameters,
							 framebuffer.size);
		
		postprocessPass(framebuffer, commandQueue);
	}
	
	void ForwardRenderer::mainPass(ForwardRendererFramebuffer& framebuffer, CommandQueue& commandQueue) const {
		std::unique_ptr _ctx = commandQueue.createRenderContext();
		auto& ctx = *_ctx;
		
		RenderPassDescription desc{};
		RenderPassColorAttachmentDescription caDesc{};
		caDesc.texture = framebuffer.rawColor;
		caDesc.clearColor = { 1, 0, 1, 1 };
		caDesc.loadAction = LoadAction::clear;
		desc.colorAttachments.push_back(caDesc);
		
		RenderPassDepthAttachmentDescription dDesc{};
		dDesc.texture = framebuffer.depth;
		
		desc.depthAttachment = dDesc;
		ctx.begin(desc);
		
		// Vertex buffers
		ctx.setVertexBuffer(renderObjects.sceneDataBuffer, 0);
		
		// Fragment buffers
		ctx.setFragmentBuffer(renderObjects.sceneDataBuffer, 0);
		ctx.setFragmentBuffer(renderObjects.shadows.dataBuffer, 1);
		ctx.setFragmentBuffer(renderObjects.shadows.dataBuffer, 2, sizeof(ShadowRenderData) /* offset */);
		ctx.setFragmentTexture(renderObjects.shadows.shadowMaps, 0);
		ctx.setFragmentSampler(renderObjects.shadows.sampler, 0);
		
		
		ctx.setVertexBuffer(renderObjects.transformBuffer, 2);
		Material* currentMaterial = nullptr;
		StaticMeshRenderer* currentMesh = nullptr;
		for (auto&& [index, object]: utl::enumerate(scene.objects)) {
			if (object.material.get() != currentMaterial) {
				currentMaterial = object.material.get();
				ctx.setPipeline(currentMaterial->mainPass);
				ctx.setTriangleCullMode(currentMaterial->cullMode);
				ctx.setDepthStencil(renderObjects.depthStencil);
			}
			if (object.mesh.get() != currentMesh) {
				ctx.setVertexBuffer(object.mesh->vertexBuffer(), 1);
			}
			// object transform
			ctx.setVertexBufferOffset(2, index * sizeof(float4x4));
			
			BufferView indexBuffer = object.mesh->indexBuffer();
			
			DrawDescription desc{};
			desc.indexCount = indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = indexBuffer;
			ctx.draw(desc);
		}
		ctx.end();
		
		ctx.commit();
	}
	
	static TextureHandle createShadowMaps(HardwareDevice& device, int totalShadowMaps, int2 resolution) {
		TextureDescription desc;
		desc.type = TextureType::texture2DArray;
		desc.size = usize3(resolution, 1);
		desc.arrayLength = totalShadowMaps;
		desc.pixelFormat = PixelFormat::Depth32Float;
		desc.usage = TextureUsage::renderTarget | TextureUsage::shaderRead;
		desc.storageMode = StorageMode::GPUOnly;
		
		return device.createTexture(desc);
	}
	
	void ForwardRenderer::shadowMapPass(CommandQueue& commandQueue) {
		if (scene.shadows.numShadowCasters == 0) {
			return;
		}
	
		std::size_t const numShadowMaps = std::accumulate(scene.shadows.numCascades.begin(),
														  scene.shadows.numCascades.end(), 0);
		
		if (numShadowMaps > scene.shadows.shadowMapArrayLength || scene.shadows.needsNewShadowMaps) {
			renderObjects.shadows.shadowMaps = createShadowMaps(device(), numShadowMaps, scene.shadows.shadowMapResolution);
			scene.shadows.shadowMapArrayLength = numShadowMaps;
			scene.shadows.needsNewShadowMaps = false;
		}
		
		std::unique_ptr _ctx = commandQueue.createRenderContext();
		auto& ctx = *_ctx;
		
		RenderPassDescription desc{};
		RenderPassDepthAttachmentDescription dDesc{};
		dDesc.texture = renderObjects.shadows.shadowMaps;
		desc.depthAttachment = dDesc;
		desc.renderTargetArrayLength = numShadowMaps;
		desc.renderTargetSize = scene.shadows.shadowMapResolution;
		
		ctx.begin(desc);
		
		ctx.setPipeline(renderObjects.shadows.pipeline);
		ctx.setTriangleCullMode(TriangleCullMode::front); /// TODO: temporary
		ctx.setDepthStencil(renderObjects.depthStencil);
		
		ctx.setVertexBuffer(renderObjects.sceneDataBuffer, 0);
		ctx.setVertexBuffer(renderObjects.transformBuffer, 2);
		ctx.setVertexBuffer(renderObjects.shadows.dataBuffer, 3,
							/* offset = */ sizeof(ShadowRenderData) /* <- this is the header of the buffer */);
		StaticMeshRenderer* currentMesh = nullptr;
		for (auto&& [index, object]: utl::enumerate(scene.objects)) {
			if (object.mesh.get() != currentMesh) {
				currentMesh = object.mesh.get();
				ctx.setVertexBuffer(currentMesh->vertexBuffer(), 1);
			}
			ctx.setVertexBufferOffset(2, index * sizeof(float4x4));
			
			BufferView indexBuffer = object.mesh->indexBuffer();
			
			DrawDescription desc{};
			desc.indexCount = indexBuffer.size() / 4;
			desc.indexType = IndexType::uint32;
			desc.indexBuffer = indexBuffer;
			desc.instanceCount = numShadowMaps;
			ctx.draw(desc);
		}
		
		ctx.end();
		ctx.commit();
	}
	
	void ForwardRenderer::postprocessPass(ForwardRendererFramebuffer& framebuffer, CommandQueue& commandQueue) const {
		std::unique_ptr const _ctx = commandQueue.createComputeContext();
		auto& ctx = *_ctx;

		ctx.begin();

		ctx.setPipeline(renderObjects.postprocessPipeline);

		ctx.setBuffer(renderObjects.rendererParameters, 0, offsetof(ForwardRendererParameters, postprocess));
		ctx.setTexture(framebuffer.postProcessed, 0); // dest
		ctx.setTexture(framebuffer.rawColor, 1);
		ctx.setTexture(framebuffer.bloom.upsampleMips.front(), 2);
		

		uint2 const gridSize = framebuffer.size;
		
		
		auto const threadGroupWidth = renderObjects.postprocessPipeline.threadExecutionWidth;
		auto const threadGroupHeight = renderObjects.postprocessPipeline.maxTotalThreadsPerThreadgroup / threadGroupWidth;
		mtl::uint2 const threadGroupSize = { threadGroupWidth, threadGroupHeight };
		
		
		ctx.dispatchThreads(gridSize, threadGroupSize);
		
		ctx.end();
		ctx.commit();
	}
	
}


