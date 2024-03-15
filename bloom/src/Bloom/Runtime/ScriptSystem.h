#ifndef BLOOM_RUNTIME_SCRIPTSYSTEM_H
#define BLOOM_RUNTIME_SCRIPTSYSTEM_H

#include <memory>

// #include <scatha/Sema/Fwd.h>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Scene/Components/Script.h"

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

#endif // BLOOM_RUNTIME_SCRIPTSYSTEM_H
