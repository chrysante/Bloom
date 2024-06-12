#ifndef BLOOM_GRAPHICS_PASSES_SHADINGPASSUTILS_H
#define BLOOM_GRAPHICS_PASSES_SHADINGPASSUTILS_H

#include "Bloom/GPU/Fwd.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/Graphics/Material/MaterialSourceData.h"

namespace bloom {

/// Provides helper functions to bind materials to a render pass
class ShadingPassHelper {
public:
    void setEncoder(RenderEncoder& encoder) { this->_encoder = &encoder; }

    void bindBuffer(BufferView buffer, GlobalMaterialBindingType bindingType,
                    MaterialShaderType shaderType);

    void bindTexture(TextureView tex, GlobalMaterialBindingType bindingType,
                     MaterialShaderType shaderType);

    void setTextureParameter(MaterialTextureParameter const& param,
                             Texture2D* tex, MaterialShaderType vertexShader,
                             MaterialShaderType fragmentShader);

    RenderEncoder& encoder() const { return *_encoder; }

private:
    RenderEncoder* _encoder;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_PASSES_SHADINGPASSUTILS_H
