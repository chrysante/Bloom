#pragma once

#include "Bloom/Core/Core.hpp"
#include "Bloom/Application/MessageSystem.hpp"
#include "Bloom/Graphics/Camera.hpp"
#include "Bloom/Graphics/Lights.hpp"
#include <mtl/mtl.hpp>

namespace bloom {
	
	class Application;
	class HardwareDevice;
	class CommandQueue;
	
	class StaticMeshRenderer;
	class Material;
	
	class BLOOM_API Framebuffer {
	public:
		Framebuffer(Framebuffer const&) = delete;
		virtual ~Framebuffer() = default;
		
		mtl::int2 size = 0;
		
	protected:
		Framebuffer() = default;
		
	};
	
	class BLOOM_API Renderer: protected Reciever {
	public:
		explicit Renderer(Reciever reciever): Reciever(std::move(reciever)) {}
		virtual ~Renderer() = default;
		
		virtual void init(HardwareDevice&) {};
		virtual std::unique_ptr<Framebuffer> createFramebuffer(mtl::int2 size) const = 0;
		virtual std::unique_ptr<Framebuffer> createDebugFramebuffer(mtl::int2 size) const = 0;
		
		virtual void beginScene(Camera const&) = 0;
		virtual void endScene() = 0;
		virtual void draw(Framebuffer&, CommandQueue&) = 0;
		
		virtual void submit(Reference<StaticMeshRenderer>, Reference<Material>, mtl::float4x4 const& transform) {};
		virtual void submit(PointLight const&) {};
		virtual void submit(SpotLight const&) {};
		virtual void submit(DirectionalLight const&) {};
		virtual void submit(SkyLight const&) {};

		HardwareDevice& device() const { return *mDevice; }

	protected:
		HardwareDevice* mDevice = nullptr;
	};

	BLOOM_API std::unique_ptr<Renderer> createForwardRenderer(Application&);
	
	struct BLOOM_API ReloadShadersCommand{};
	
}



