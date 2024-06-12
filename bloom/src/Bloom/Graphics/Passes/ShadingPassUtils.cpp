#include "Bloom/Graphics/Passes/ShadingPassUtils.h"

#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Texture.h"

using namespace bloom;

void ShadingPassHelper::bindBuffer(BufferView buffer,
                                   GlobalMaterialBindingType bindingType,
                                   MaterialShaderType shaderType) {
    auto [resourceType, index] =
        getGlobalMaterialBindingPoint(shaderType, bindingType);
    BL_ASSERT(resourceType == MaterialBindingPoint::Buffer);
    switch (shaderType) {
    case MaterialShaderType::ShadingVertex:
        [[fallthrough]];
    case MaterialShaderType::ShadowVertex:
        encoder().setVertexBuffer(buffer, index);
        break;
    case MaterialShaderType::ShadingFragment:
        [[fallthrough]];
    case MaterialShaderType::ShadowFragment:
        encoder().setFragmentBuffer(buffer, index);
        break;
    default:
        BL_UNREACHABLE();
    }
}

void ShadingPassHelper::bindTexture(TextureView tex,
                                    GlobalMaterialBindingType bindingType,
                                    MaterialShaderType shaderType) {
    auto [resourceType, index] =
        getGlobalMaterialBindingPoint(shaderType, bindingType);
    BL_ASSERT(resourceType == MaterialBindingPoint::Texture);
    switch (shaderType) {
    case MaterialShaderType::ShadingVertex:
        [[fallthrough]];
    case MaterialShaderType::ShadowVertex:
        encoder().setVertexTexture(tex, index);
        break;
    case MaterialShaderType::ShadingFragment:
        [[fallthrough]];
    case MaterialShaderType::ShadowFragment:
        encoder().setFragmentTexture(tex, index);
        break;
    default:
        BL_UNREACHABLE();
    }
}

void ShadingPassHelper::setTextureParameter(
    MaterialTextureParameter const& param, Texture2D* tex,
    MaterialShaderType vertexShader, MaterialShaderType fragmentShader) {
    if (param.hasBinding(vertexShader)) {
        auto texView = tex ? tex->GPUHandle : TextureView();
        encoder().setVertexTexture(texView, param.bindingPoint[vertexShader]);
    }
    if (param.hasBinding(fragmentShader)) {
        auto texView = tex ? tex->GPUHandle : TextureView();
        encoder().setFragmentTexture(texView,
                                     param.bindingPoint[fragmentShader]);
    }
}
