#include "Bloom/Platform/Metal/MetalRenderContext.h"

#include "Bloom/Platform/Metal/MetalSwapchain.h"

using namespace mtl::short_types;
using namespace bloom;
	
MetalRenderContext::MetalRenderContext(id<MTLCommandBuffer> cb):
    commandBuffer(cb)
{
    
}

void MetalRenderContext::begin(RenderPassDescription const& desc) {
    MTLRenderPassDescriptor* mtlDesc = [[MTLRenderPassDescriptor alloc] init];
    
    for (size_t index = 0; auto ca: desc.colorAttachments) {
        MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = [[MTLRenderPassColorAttachmentDescriptor alloc] init];
        colorAttachmentDesc.texture = (__bridge id<MTLTexture>)ca.texture.nativeHandle();
        float4 const cc = ca.clearColor;
        colorAttachmentDesc.clearColor  = MTLClearColorMake(cc.r, cc.g, cc.b, cc.a);
        colorAttachmentDesc.loadAction  = (MTLLoadAction)utl::to_underlying(ca.loadAction);
        colorAttachmentDesc.storeAction = (MTLStoreAction)utl::to_underlying(ca.storeAction);
        
        [mtlDesc.colorAttachments setObject:colorAttachmentDesc
                         atIndexedSubscript:index];
        ++index;
    }
    
    {
        MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc = [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
        depthAttachmentDesc.texture = (__bridge id<MTLTexture>)desc.depthAttachment.texture.nativeHandle();
        auto const& da = desc.depthAttachment;
        depthAttachmentDesc.clearDepth  = da.clearDepth;
        depthAttachmentDesc.loadAction  = (MTLLoadAction)utl::to_underlying(da.loadAction);
        depthAttachmentDesc.storeAction = (MTLStoreAction)utl::to_underlying(da.storeAction);
        
        mtlDesc.depthAttachment = depthAttachmentDesc;
    }
    
    mtlDesc.renderTargetArrayLength = desc.renderTargetArrayLength;
    mtlDesc.renderTargetWidth = desc.renderTargetSize.x;
    mtlDesc.renderTargetHeight = desc.renderTargetSize.y;
    
    commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:mtlDesc];
}

void MetalRenderContext::end() {
    [commandEncoder endEncoding];
}

void MetalRenderContext::setPipeline(RenderPipelineView pipeline) {
    [commandEncoder setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)pipeline.nativeHandle()];
}

void MetalRenderContext::setDepthStencil(DepthStencilView depthStencil) {
    [commandEncoder setDepthStencilState:(__bridge id<MTLDepthStencilState>)depthStencil.nativeHandle()];
}

void MetalRenderContext::setVertexBuffer(BufferView buffer, std::size_t index, std::size_t offset) {
    [commandEncoder setVertexBuffer:(__bridge id<MTLBuffer>)buffer.nativeHandle()
                             offset:offset
                            atIndex:index];
}

void MetalRenderContext::setVertexBufferOffset(std::size_t index, std::size_t offset) {
    [commandEncoder setVertexBufferOffset:offset
                                  atIndex:index];
}

void MetalRenderContext::setVertexTexture(TextureView texture, std::size_t index) {
    [commandEncoder setVertexTexture:(__bridge id<MTLTexture>)texture.nativeHandle()
                             atIndex:index];
}

void MetalRenderContext::setVertexSampler(SamplerView sampler, std::size_t index) {
    [commandEncoder setVertexSamplerState:(__bridge id<MTLSamplerState>)sampler.nativeHandle()
                                  atIndex:index];
}

void MetalRenderContext::setFragmentBuffer(BufferView buffer, std::size_t index, std::size_t offset) {
    [commandEncoder setFragmentBuffer:(__bridge id<MTLBuffer>)buffer.nativeHandle()
                               offset:offset
                              atIndex:index];
}

void MetalRenderContext::setFragmentBufferOffset(std::size_t index, std::size_t offset) {
    [commandEncoder setFragmentBufferOffset:offset
                                    atIndex:index];
}

void MetalRenderContext::setFragmentTexture(TextureView texture, std::size_t index) {
    [commandEncoder setFragmentTexture:(__bridge id<MTLTexture>)texture.nativeHandle()
                               atIndex:index];
}

void MetalRenderContext::setFragmentSampler(SamplerView sampler, std::size_t index) {
    [commandEncoder setFragmentSamplerState:(__bridge id<MTLSamplerState>)sampler.nativeHandle()
                                    atIndex:index];
}

void MetalRenderContext::setTriangleFillMode(TriangleFillMode mode) {
    [commandEncoder setTriangleFillMode:(MTLTriangleFillMode)mode];
}

void MetalRenderContext::setTriangleCullMode(TriangleCullMode mode) {
    [commandEncoder setCullMode:(MTLCullMode)mode];
}

void MetalRenderContext::draw(DrawDescription const& desc) {
    [commandEncoder drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                               indexCount: desc.indexCount
                                indexType: (MTLIndexType)desc.indexType
                              indexBuffer: (__bridge id<MTLBuffer>)desc.indexBuffer.nativeHandle()
                        indexBufferOffset: desc.indexBufferOffset
                            instanceCount: desc.instanceCount];
}

void MetalRenderContext::draw(std::size_t vertexStart, std::size_t vertexCount) {
    [commandEncoder drawPrimitives: MTLPrimitiveTypeTriangle
                       vertexStart: vertexStart
                       vertexCount: vertexCount];
}

void MetalRenderContext::present(Backbuffer& bb) {
    auto& mtlBackbuffer = utl::down_cast<MetalBackbuffer&>(bb);
    [commandBuffer presentDrawable: mtlBackbuffer.drawable];
}

void MetalRenderContext::commit() {
    [commandBuffer commit];
}
