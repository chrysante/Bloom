#ifndef POPPY_EDITOR_VIEWS_RENDERERDEBUGGERVIEW_H
#define POPPY_EDITOR_VIEWS_RENDERERDEBUGGERVIEW_H

#include <memory>

#include "Bloom/Graphics/Renderer/ForwardRenderer.h"
#include "Poppy/UI/View.h"

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
        int upsampleMipmapLevel = 0;
    } veil;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_RENDERERDEBUGGERVIEW_H
