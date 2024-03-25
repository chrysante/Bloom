#ifndef BLOOM_GRAPHICS_RENDERER_RENDERER_H
#define BLOOM_GRAPHICS_RENDERER_RENDERER_H

#include <vml/vml.hpp>

#include "Bloom/Application/MessageSystem.h"
#include "Bloom/Core/Core.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Graphics/Lights.h"

namespace bloom {

class Application;
class HardwareDevice;
class CommandQueue;

class StaticMeshRenderer;
class MaterialInstance;

class BLOOM_API Framebuffer {
public:
    Framebuffer(Framebuffer const&) = delete;

    virtual ~Framebuffer() = default;

    vml::int2 size = 0;

protected:
    Framebuffer() = default;
};

class BLOOM_API Renderer: protected Receiver {
public:
    explicit Renderer(Receiver receiver): Receiver(std::move(receiver)) {}
    virtual ~Renderer() = default;

    virtual void init(HardwareDevice&){};
    virtual std::unique_ptr<Framebuffer> createFramebuffer(
        vml::int2 size) const = 0;
    virtual std::unique_ptr<Framebuffer> createDebugFramebuffer(
        vml::int2 size) const = 0;

    virtual void beginScene(Camera const&) = 0;
    virtual void endScene() = 0;
    virtual void draw(Framebuffer&, CommandQueue&) = 0;

    virtual void submit(Reference<StaticMeshRenderer>,
                        Reference<MaterialInstance>,
                        [[maybe_unused]] vml::float4x4 const& transform){};
    virtual void submit(PointLight const&){};
    virtual void submit(SpotLight const&){};
    virtual void submit(DirectionalLight const&){};
    virtual void submit(SkyLight const&){};

    HardwareDevice& device() const { return *mDevice; }

protected:
    HardwareDevice* mDevice = nullptr;
};

BLOOM_API std::unique_ptr<Renderer> createForwardRenderer(Application&);

struct BLOOM_API ReloadShadersCommand {};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_RENDERER_H
