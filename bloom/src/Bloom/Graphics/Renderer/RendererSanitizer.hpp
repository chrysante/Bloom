#pragma once

#include "Bloom/Core/Core.hpp"

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
    bool isInit        = false;
};

} // namespace bloom
