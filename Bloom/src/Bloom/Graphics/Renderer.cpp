#define UTL_DEFER_MACROS

#include "Renderer.hpp"

#include <algorithm>
#include <array>
#include <utl/scope_guard.hpp>

#include "Bloom/Core/Debug.hpp"
#include "SceneRenderData.hpp"

#include "RenderContext.hpp"
#include "FrameBuffer.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "StaticMesh.hpp"
#include "RenderContext.hpp"

#include "Bloom/Platform/Metal/MetalRenderContext.hpp"
#include <Metal/Metal.hpp>

namespace bloom {

	std::string_view toString(DebugDrawMode mode) {
		return std::array{
			"Lit",
			"Wireframe"
		}[(std::size_t)mode];
	}
	
	std::ostream& operator<<(std::ostream& stream, DebugDrawMode mode) {
		return stream << toString(mode);
	}
	
	void Renderer::init(RenderContext* renderContext) {
		this->renderContext = renderContext;
		sceneDataBuffer = renderContext->createUniformBuffer(nullptr, sizeof(SceneRenderData));
		depthStencil = renderContext->createDepthStencilState(CompareFunction::lessEqual);
		createPostprocessQuad();
		createPostprocessPipelines();
		createPostprocessSampler();
	}
	
	
	void Renderer::beginScene(Camera const& camera) {
		bloomExpect(!buildingScene, "Already called beginScene?");
		buildingScene = true;
		this->camera = camera;
		objects.clear();
		pointLights.clear();
		spotLights.clear();
	}
	
	void Renderer::endScene() {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		buildingScene = false;
	}
	
	void Renderer::submit(StaticMesh* mesh, Material* material, EntityRenderData entityData, bool selected) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		entityData.transform = mtl::transpose(entityData.transform);
		objects.push_back({ entityData, selected, material, mesh });
	}
	
	void Renderer::submit(PointLight light, mtl::float3 position) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		pointLights.push_back({ light, .position = position });
	}
	
	void Renderer::submit(SpotLight light, mtl::float3 position, mtl::float3 direction) {
		bloomExpect(buildingScene, "Forgot to call beginScene?");
		light.innerCutoff = std::cos(light.innerCutoff);
		light.outerCutoff = std::cos(light.outerCutoff);
		spotLights.push_back({ light, .position = position, .direction = direction });
	}
	
	static auto const objectOrder = [](auto&& a, auto&& b) {
		if (a.material == b.material) {
			return a.mesh < b.mesh;
		}
		return a.material < b.material;
	};
	
	void Renderer::debugDraw(EditorFrameBuffer* frameBuffer, DebugDrawMode mode) {
		std::sort(objects.begin(), objects.end(), objectOrder);
		uploadEntityData();
		SceneRenderData sceneRenderData;
		
		// Scene Data
		sceneRenderData.camera = mtl::transpose(camera.getMatrix());
		sceneRenderData.cameraPosition = camera.getPosition();
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
		
		// Editor Properties
		sceneRenderData.selectionLineWidth = 3;
		
		renderContext->fillBuffer(sceneDataBuffer, &sceneRenderData, sizeof sceneRenderData);
		
		
		auto mainHandle = mainPassEditor(frameBuffer, mode);
		auto outlineHandle = outlinePass(frameBuffer);
		mainHandle.wait();
		outlineHandle.wait();
		
		postprocess(frameBuffer);
		editorPP(frameBuffer);
	}
	
	void Renderer::postprocess(EditorFrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->finalImage());
		
		renderContext->beginRenderPass();
		
		renderContext->setPipelineState(postprocessPipeline);
		renderContext->setVertexBuffer(quadVB, 1);
		
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentTexture(frameBuffer->color(), 0);
		renderContext->setFragmentSampler(postprocessSampler, 0);
		
		renderContext->drawIndexed(quadIB, bloom::IndexType::uint32);
		
		renderContext->commit().wait();
	}
	
	RenderPassHandle Renderer::mainPassEditor(EditorFrameBuffer* frameBuffer, DebugDrawMode mode) {
		renderContext->setRenderTargetColor(0, frameBuffer->color());
		switch (mode) {
			case DebugDrawMode::lit:
				renderContext->setClearColor(0, mtl::float4{ 0, 0, 0, 1 });
				break;
			
			case DebugDrawMode::wireframe:
				renderContext->setClearColor(0, mtl::float4{ 0, 0, 0, 1 });
				break;
				
			default:
				break;
		}
		
		renderContext->setRenderTargetColor(1, frameBuffer->entityID());
		renderContext->setClearColor(1, 0xFFFFFFFF /* null entity */);
		
		renderContext->setRenderTargetColor(2, frameBuffer->selected());
		renderContext->setClearColor(2, 0);
		
		renderContext->setRenderTargetDepth(frameBuffer->depth());
		
		Material* currentMaterial = nullptr;
		
		renderContext->beginRenderPass();
		
		renderContext->setVertexBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		
		if (mode == DebugDrawMode::wireframe) {
			renderContext->setTriangleFillMode(TriangleFillMode::lines);
		}
		
		for (std::size_t index = 0;
			 auto object: objects)
		{
			utl_defer { ++index; };
			if (object.material != currentMaterial) {
				currentMaterial = object.material;
				renderContext->setPipelineState(currentMaterial->mainPassEditor);
				renderContext->setDepthStencilState(depthStencil);
			}
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			renderContext->drawIndexed(object.mesh->indexBuffer, bloom::IndexType::uint32);
		}
		
		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::outlinePass(EditorFrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->selected());
		renderContext->setClearColor(0, 0.0f);
		
		Material* currentMaterial = nullptr;
		
		renderContext->beginRenderPass();
		
		renderContext->setVertexBuffer(sceneDataBuffer, 0);
	
		for (std::size_t index = 0;
			 auto object: objects)
		{
			utl_defer { ++index; };
			if (!object.selected) {
				continue;
			}
			if (object.material != currentMaterial) {
				currentMaterial = object.material;
				renderContext->setPipelineState(currentMaterial->outlinePass);
			}
			renderContext->setVertexBuffer(object.mesh->vertexBuffer, 1);
			renderContext->setVertexBuffer(entityDataBuffer, 2, index * sizeof(EntityRenderData));
			renderContext->drawIndexed(object.mesh->indexBuffer, bloom::IndexType::uint32);
		}
		
		return renderContext->commit();
	}
	
	RenderPassHandle Renderer::editorPP(EditorFrameBuffer* frameBuffer) {
		renderContext->setRenderTargetColor(0, frameBuffer->_finalImageEditorBuffer);
		
		renderContext->beginRenderPass();
		
		renderContext->setPipelineState(editorPPPipeline);
		renderContext->setVertexBuffer(quadVB, 1);
		
		renderContext->setFragmentBuffer(sceneDataBuffer, 0);
		renderContext->setFragmentTexture(frameBuffer->finalImage(), 0);
		renderContext->setFragmentTexture(frameBuffer->_selectedBuffer, 1);
		renderContext->setFragmentSampler(postprocessSampler, 0);
		
		renderContext->drawIndexed(quadIB, bloom::IndexType::uint32);
		
		return renderContext->commit();
	}
	
	void Renderer::uploadEntityData() {
		std::size_t const size = objects.size() * sizeof(EntityRenderData);
		if (entityDataBuffer.size() < size) {
			entityDataBuffer = renderContext->createUniformBuffer(nullptr, size);
		}
		return renderContext->fillBuffer(entityDataBuffer, objects.data().entity, size);
	}
	
	void Renderer::createPostprocessQuad() {
		mtl::float2 vertices[] = {
			{ -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
		};
		quadVB = renderContext->createVertexBuffer(vertices, sizeof vertices);
		
		std::uint32_t indices[] = {
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
			
			postprocessPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
			
			errors->release();
		}
		
		/* editorPPPipeline */ {
			ARCPointer psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
			psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("postprocessVS")));
			psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("editorPP")));

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc.get(), &errors);
			assert(ps);
			
			editorPPPipeline = bloom::RenderPipelineHandle(ps, MTLDeleter);
			
			errors->release();
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
