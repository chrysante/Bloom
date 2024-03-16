#ifndef BLOOM_RUNTIME_SCRIPTSYSTEM_H
#define BLOOM_RUNTIME_SCRIPTSYSTEM_H

#include <memory>

#include <scatha/Sema/Fwd.h>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Scene/Components/Script.h"

namespace bloom {

class Scene;

///
class BLOOM_API ScriptSystem: public CoreSystem {
public:
    ScriptSystem();

    ~ScriptSystem();

    void init();

    void onSceneConstruction(Scene& scene);

    void onSceneInit(Scene& scene);

    void onSceneUpdate(Scene& scene, Timestep timestep);

    void onSceneRender(Scene& scene);

    ///
    scatha::sema::SymbolTable const* symbolTable() const;

    struct Impl;

private:
    friend class AssetManager;

    void scriptsWillCompile();
    void scriptsDidCompile();

    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_SCRIPTSYSTEM_H
