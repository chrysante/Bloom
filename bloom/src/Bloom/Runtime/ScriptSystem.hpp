#pragma once

#include <memory>

// #include <scatha/Sema/Fwd.h>

#include "Bloom/Application/CoreSystem.hpp"
#include "Bloom/Core/Time.hpp"
#include "Bloom/Scene/Components/Script.hpp"

namespace bloom {

class BLOOM_API ScriptSystem: public CoreSystem {
public:
    ScriptSystem();

    ~ScriptSystem();

    void init();

    void onSceneConstruction();

    void onSceneInit();

    void onSceneUpdate(Timestep);

    void onSceneRender();

    //    void* instantiateObject(scatha::sema::StructType const* classType);

private:
    struct Impl;

    void scriptsWillLoad();
    void scriptsDidLoad();
    void forEach(auto&& fn);

    std::unique_ptr<Impl> impl;
};

} // namespace bloom
