#pragma once

#include <memory>

#include "Bloom/Graphics/Renderer/ForwardRenderer.hpp"
#include "Poppy/UI/View.hpp"

namespace poppy {

class RendererDebuggerView: public View {
public:
    void init() override;
    void frame() override;

private:
    bloom::ForwardRenderer* findRenderer() const;
    std::shared_ptr<bloom::ForwardRendererFramebuffer> findFramebuffer() const;

private:
    bloom::ForwardRenderer* mRenderer = nullptr;
    std::weak_ptr<bloom::ForwardRendererFramebuffer> mFramebuffer;
    std::shared_ptr<bloom::ForwardRendererFramebuffer> currentFramebuffer;

private:
    struct BloomVeilParams {
        int downsampleMipmapLevel = 0;
        int upsampleMipmapLevel   = 0;
    } veil;
};

} // namespace poppy
