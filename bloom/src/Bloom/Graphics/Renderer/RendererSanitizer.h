#ifndef BLOOM_GRAPHICS_RENDERER_RENDERERSANITIZER_H
#define BLOOM_GRAPHICS_RENDERER_RENDERERSANITIZER_H

#include "Bloom/Core/Core.h"

namespace bloom {

class RendererSanitizer {
public:
    void init() { isInit = true; }
    void beginScene() {
        assert(isInit);
        buildingScene = true;
    }
    void endScene() {
        assert(isInit);
        buildingScene = false;
    }
    void submit() {
        assert(isInit);
        assert(buildingScene);
    }

private:
    bool buildingScene = false;
    bool isInit = false;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_RENDERERSANITIZER_H