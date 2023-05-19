#pragma once

#include <memory>

#include "Bloom/Application/CoreSystem.hpp"
#include "Bloom/Core/Time.hpp"

namespace bloom {

struct ScriptLoadEvent {};

class BLOOM_API ScriptSystem: public CoreSystem {
public:
    ScriptSystem();

    ~ScriptSystem();

    void init();

    void onSceneConstruction();

    void onSceneInit();

    void onSceneUpdate(Timestep);

    void onSceneRender();

private:
    void onScriptReload();

    void forEach(auto&& fn);

    struct Impl;

    std::unique_ptr<Impl> impl;
};

} // namespace bloom
