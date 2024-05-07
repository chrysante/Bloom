#ifndef BLOOM_GPU_FWD_H
#define BLOOM_GPU_FWD_H

namespace bloom {

/// List of supported render APIs
enum class RenderAPI { Metal };

class HardwareDevice;
class CommandQueue;
class Swapchain;
class Backbuffer;
class CommandBuffer;
class CommandEncoder;
class RenderEncoder;
class ComputeEncoder;
class BlitEncoder;

struct RenderPassDescription;
struct DrawDescription;
struct RenderPassDepthAttachmentDescription;
struct RenderPassColorAttachmentDescription;

class BufferHandle;
class BufferView;
class TextureHandle;
class TextureView;
class ShaderFunctionHandle;
class ShaderFunctionView;
class RenderPipelineHandle;
class RenderPipelineView;
class ComputePipelineHandle;
class ComputePipelineView;
class DepthStencilHandle;
class DepthStencilView;
class SamplerHandle;
class SamplerView;

} // namespace bloom

#endif // BLOOM_GPU_FWD_H
